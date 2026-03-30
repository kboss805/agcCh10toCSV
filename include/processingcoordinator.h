/**
 * @file processingcoordinator.h
 * @brief Orchestrates worker thread lifecycle and batch processing sequencing.
 *
 * Owns all transient processing state: the background QThread, the
 * FrameProcessor instance, progress tracking, and the batch state machine.
 * MainViewModel constructs one of these, passes non-owning pointers to the
 * shared model objects, and wires its signals into the ViewModel's own signals.
 */

#ifndef PROCESSINGCOORDINATOR_H
#define PROCESSINGCOORDINATOR_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>

#include "batchfileinfo.h"
#include "processingparams.h"

class Chapter10Reader;
class FrameProcessor;
class FrameSetup;

/**
 * @brief Owns the worker thread lifecycle and batch processing state machine.
 *
 * Constructed by MainViewModel as a child QObject (so it is destroyed before
 * the ViewModel's own member variables). Holds non-owning pointers to the
 * shared Chapter10Reader, FrameSetup, and batch file list — all of which are
 * owned by MainViewModel and outlive the coordinator.
 */
class ProcessingCoordinator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the coordinator with non-owning pointers to shared state.
     * @param batch_files  Pointer to ViewModel's m_batch_files vector.
     * @param frame_setup  Pointer to ViewModel's FrameSetup instance.
     * @param reader       Pointer to ViewModel's Chapter10Reader instance.
     * @param parent       QObject parent (should be the owning MainViewModel).
     */
    explicit ProcessingCoordinator(QVector<BatchFileInfo>* batch_files,
                                   FrameSetup*             frame_setup,
                                   Chapter10Reader*        reader,
                                   QObject*                parent = nullptr);
    ~ProcessingCoordinator();

    ProcessingCoordinator(const ProcessingCoordinator&)            = delete;
    ProcessingCoordinator& operator=(const ProcessingCoordinator&) = delete;
    ProcessingCoordinator(ProcessingCoordinator&&)                 = delete;
    ProcessingCoordinator& operator=(ProcessingCoordinator&&)      = delete;

    // -----------------------------------------------------------------------
    // Processing entry points — called by MainViewModel after validation
    // -----------------------------------------------------------------------

    /**
     * @brief Starts single-file processing.
     * @p params must be fully validated. @p receiver_states is used to enable
     * the correct frame parameters before launching the worker thread.
     * Returns false (and emits errorOccurred) if no receivers are selected.
     */
    bool startSingleProcessing(const ProcessingParams&        params,
                               const QVector<QVector<bool>>& receiver_states);

    /**
     * @brief Starts batch processing.
     *
     * Pre-scans all valid batch files, then launches the batch state machine.
     * Settings values are captured at call time and held constant for the run.
     */
    void startBatchProcessing(const QString&              output_dir,
                              int                         sample_rate_index,
                              const QString&              frame_sync_str,
                              int                         polarity_idx,
                              int                         slope_idx,
                              const QString&              scale_str,
                              const QVector<QVector<bool>>& receiver_states);

    /**
     * @brief Re-queues failed batch files and re-runs processing.
     * Settings values are re-captured so any user changes take effect.
     */
    void retryFailedFiles(const QString&              frame_sync_str,
                          int                         polarity_idx,
                          int                         slope_idx,
                          const QString&              scale_str,
                          const QVector<QVector<bool>>& receiver_states);

    /// Requests abort of the active processor and flags the batch as cancelled.
    void cancelProcessing();

    /**
     * @brief Pre-scans a single file to detect encoding and verify sync.
     *
     * Updates isRandomized(). Called by MainViewModel for the single-file
     * mode UI indicator when the user changes the PCM channel, and internally
     * before single-file processing starts.
     *
     * @param pcm_channel_id  Resolved PCM channel ID to scan.
     * @param filename        Path to the .ch10 file.
     * @param frame_sync_str  Frame sync hex string from settings.
     * @return true if at least one sync pattern was found.
     */
    bool runPreScan(int            pcm_channel_id,
                    const QString& filename,
                    const QString& frame_sync_str);

    /// Resets transient batch counters/index. Called by MainViewModel::clearState().
    void reset();

    // -----------------------------------------------------------------------
    // State queries — used by MainViewModel property getters
    // -----------------------------------------------------------------------
    bool  processing()      const;  ///< @return True while background processing is active.
    int   progressPercent() const;  ///< @return Current processing progress (0--100).
    bool  isRandomized()    const;  ///< @return True if last preScan detected RNRZ-L encoding.

signals:
    /// Emitted when processing progress changes. ViewModel re-emits progressPercentChanged().
    void progressChanged(int percent);
    /// Emitted when active processing state changes.
    /// ViewModel re-emits processingChanged() + controlsEnabledChanged().
    void processingStateChanged(bool active);
    /// Emitted when the batch moves to a new file.
    void batchFileProcessing(int file_index, int total);
    /// Emitted after the coordinator mutates the batch file list.
    /// ViewModel rebuilds the status summary and re-emits batchFilesChanged().
    void batchFilesUpdated();
    /// Emitted when processing finishes (single file or full batch).
    void processingFinished(bool success, const QString& output_path);
    /// Forwarded log message from the worker thread.
    void logMessageReceived(const QString& message);
    /// Emitted on processing error.
    void errorOccurred(const QString& message);

private:
    /// Applies calibration slope/scale to each enabled frame parameter.
    bool prepareFrameSetupParameters(const CalibrationParams& cal);
    /// Builds a name-to-index map for O(1) parameter lookup in the frame setup.
    QMap<QString, int> buildParameterMap() const;

    /// Creates a FrameProcessor and starts it on a background thread.
    void launchWorkerThread(const ProcessingParams& params);
    /// Runs pre-scan on all valid, unprocessed batch files.
    void preScanBatchFiles();
    /// Advances the batch state machine to the next file.
    void processNextBatchFile();

    // Slots connected to FrameProcessor signals
    void onProgressUpdated(int percent);
    void onProcessingFinished(bool success);
    void onLogMessage(const QString& message);

    // Non-owning pointers — lifetime guaranteed by ViewModel (parent QObject)
    QVector<BatchFileInfo>* m_batch_files;
    FrameSetup*             m_frame_setup;
    Chapter10Reader*        m_reader;

    // Thread lifecycle
    QThread*        m_worker_thread     = nullptr;
    FrameProcessor* m_current_processor = nullptr;

    // Processing state
    bool    m_processing       = false;
    int     m_progress_percent = 0;
    bool    m_is_randomized    = false;
    QString m_last_output_file;

    // Batch state machine
    int     m_batch_current_index    = 0;
    bool    m_batch_cancelled        = false;
    QString m_batch_output_dir;
    int     m_batch_success_count    = 0;
    int     m_batch_skip_count       = 0;
    int     m_batch_error_count      = 0;
    int     m_batch_sample_rate_index = 0;

    // Settings snapshot — captured at run start, constant for the duration
    QString                m_frame_sync_str;
    int                    m_polarity_idx  = 0;
    int                    m_slope_idx     = 0;
    QString                m_scale_str;
    QVector<QVector<bool>> m_receiver_states;
};

#endif // PROCESSINGCOORDINATOR_H
