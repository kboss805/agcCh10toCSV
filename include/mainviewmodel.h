/**
 * @file mainviewmodel.h
 * @brief ViewModel mediating between the View (MainView) and Model layer.
 */

#ifndef MAINVIEWMODEL_H
#define MAINVIEWMODEL_H

#include <QObject>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVector>

#include "batchfileinfo.h"
#include "settingsdata.h"

class Chapter10Reader;
class FrameSetup;
class FrameProcessor;
class SettingsManager;

/**
 * @brief Mediates between the View (MainView) and Model layer.
 *
 * Owns all application state, validation, processing orchestration,
 * and settings coordination. The View binds to Q_PROPERTYs and
 * connects signals; it never performs business logic.
 */
class MainViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString inputFilename READ inputFilename NOTIFY inputFilenameChanged)
    Q_PROPERTY(QStringList timeChannelList READ timeChannelList NOTIFY channelListsChanged)
    Q_PROPERTY(QStringList pcmChannelList READ pcmChannelList NOTIFY channelListsChanged)
    Q_PROPERTY(int timeChannelIndex READ timeChannelIndex WRITE setTimeChannelIndex NOTIFY timeChannelIndexChanged)
    Q_PROPERTY(int pcmChannelIndex READ pcmChannelIndex WRITE setPcmChannelIndex NOTIFY pcmChannelIndexChanged)
    Q_PROPERTY(bool fileLoaded READ fileLoaded NOTIFY fileLoadedChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressPercentChanged)
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)
    Q_PROPERTY(bool controlsEnabled READ controlsEnabled NOTIFY controlsEnabledChanged)

    Q_PROPERTY(int startDayOfYear READ startDayOfYear NOTIFY fileTimesChanged)
    Q_PROPERTY(int startHour READ startHour NOTIFY fileTimesChanged)
    Q_PROPERTY(int startMinute READ startMinute NOTIFY fileTimesChanged)
    Q_PROPERTY(int startSecond READ startSecond NOTIFY fileTimesChanged)
    Q_PROPERTY(int stopDayOfYear READ stopDayOfYear NOTIFY fileTimesChanged)
    Q_PROPERTY(int stopHour READ stopHour NOTIFY fileTimesChanged)
    Q_PROPERTY(int stopMinute READ stopMinute NOTIFY fileTimesChanged)
    Q_PROPERTY(int stopSecond READ stopSecond NOTIFY fileTimesChanged)

    Q_PROPERTY(bool extractAllTime READ extractAllTime WRITE setExtractAllTime NOTIFY extractAllTimeChanged)
    Q_PROPERTY(int sampleRateIndex READ sampleRateIndex WRITE setSampleRateIndex NOTIFY sampleRateIndexChanged)
    Q_PROPERTY(QString frameSync READ frameSync WRITE setFrameSync NOTIFY settingsChanged)
    Q_PROPERTY(int polarityIndex READ polarityIndex WRITE setPolarityIndex NOTIFY settingsChanged)
    Q_PROPERTY(int slopeIndex READ slopeIndex WRITE setSlopeIndex NOTIFY settingsChanged)
    Q_PROPERTY(QString scale READ scale WRITE setScale NOTIFY settingsChanged)
    Q_PROPERTY(int receiverCount READ receiverCount WRITE setReceiverCount NOTIFY receiverLayoutChanged)
    Q_PROPERTY(int channelsPerReceiver READ channelsPerReceiver WRITE setChannelsPerReceiver NOTIFY receiverLayoutChanged)

    Q_PROPERTY(bool batchMode READ batchMode NOTIFY batchModeChanged)
    Q_PROPERTY(int batchFileCount READ batchFileCount NOTIFY batchFilesChanged)

public:
    explicit MainViewModel(QObject* parent = nullptr);
    ~MainViewModel();

    /// @name Property getters
    /// @{
    QString inputFilename() const;              ///< @return Path to the loaded .ch10 file.
    QStringList timeChannelList() const;         ///< @return Display strings for time channel combo box.
    QStringList pcmChannelList() const;          ///< @return Display strings for PCM channel combo box.
    int timeChannelIndex() const;                ///< @return Currently selected time channel index.
    int pcmChannelIndex() const;                 ///< @return Currently selected PCM channel index.
    bool fileLoaded() const;                     ///< @return True if a .ch10 file is loaded.
    int progressPercent() const;                 ///< @return Current processing progress (0--100).
    bool processing() const;                     ///< @return True while background processing is active.
    bool controlsEnabled() const;                ///< @return True when UI controls should be interactive.

    bool extractAllTime() const;                 ///< @return True if the full time range should be extracted.
    int sampleRateIndex() const;                 ///< @return Sample rate combo box index.

    int startDayOfYear() const;                  ///< @return File start day-of-year.
    int startHour() const;                       ///< @return File start hour.
    int startMinute() const;                     ///< @return File start minute.
    int startSecond() const;                     ///< @return File start second.
    int stopDayOfYear() const;                   ///< @return File stop day-of-year.
    int stopHour() const;                        ///< @return File stop hour.
    int stopMinute() const;                      ///< @return File stop minute.
    int stopSecond() const;                      ///< @return File stop second.

    QString frameSync() const;                   ///< @return Frame sync hex string.
    int polarityIndex() const;                    ///< @return Polarity combo box index (0=Positive, 1=Negative).
    int slopeIndex() const;                      ///< @return Voltage slope combo box index.
    QString scale() const;                       ///< @return Calibration scale in dB per volt.
    int receiverCount() const;                   ///< @return Number of receivers.
    int channelsPerReceiver() const;             ///< @return Number of channels per receiver.
    /// @}

    /// @name Property setters
    /// @{
    void setTimeChannelIndex(int index);         ///< Sets the selected time channel index.
    void setPcmChannelIndex(int index);           ///< Sets the selected PCM channel index.
    void setExtractAllTime(bool value);           ///< Sets whether to extract the full time range.
    void setSampleRateIndex(int value);           ///< Sets the sample rate combo box index.
    void setFrameSync(const QString& value);      ///< Sets the frame sync hex string.
    void setPolarityIndex(int value);              ///< Sets the polarity combo box index.
    void setSlopeIndex(int value);                ///< Sets the voltage slope combo box index.
    void setScale(const QString& value);          ///< Sets the calibration scale in dB per volt.
    void setReceiverCount(int value);             ///< Sets the number of receivers and resizes the grid.
    void setChannelsPerReceiver(int value);       ///< Sets channels per receiver and resizes the grid.
    /// @}

    /// @name Batch processing getters
    /// @{
    bool batchMode() const;                              ///< @return True when multiple files are loaded.
    int batchFileCount() const;                          ///< @return Total number of files in the batch.
    int batchValidCount() const;                         ///< @return Number of valid (non-skipped) files.
    int batchSkippedCount() const;                       ///< @return Number of skipped files.
    const QVector<BatchFileInfo>& batchFiles() const;    ///< @return Read-only access to the batch file list.
    /// @return Auto-generated output filename for batch mode (AGC_<basename>.csv).
    QString generateBatchOutputFilename(const QString& input_filepath) const;
    /// @return Formatted status summary for the file list tree header.
    QString batchStatusSummary() const;
    /// @}

    /// @name Receiver grid state
    /// @{

    /// @return True if the specified receiver/channel is checked.
    bool receiverChecked(int receiver_index, int channel_index) const;
    /// Sets the checked state of a single receiver/channel cell.
    void setReceiverChecked(int receiver_index, int channel_index, bool checked);
    /// Sets all receiver/channel cells to @p checked.
    void setAllReceiversChecked(bool checked);
    /// @}

    /// @name Settings integration
    /// @{

    /// @return Snapshot of all UI-relevant state for serialization.
    SettingsData getSettingsData() const;
    /// Restores UI state from a previously saved snapshot.
    void applySettingsData(const SettingsData& data);
    /// Loads frame parameter definitions from an INI file.
    void loadFrameSetupFrom(const QString& filename);
    /// Writes the current frame parameter definitions to @p settings.
    void saveFrameSetupTo(QSettings& settings);
    /// @}

    /// @name Helpers
    /// @{

    /// @return Channel prefix string ("L", "R", "C", ...) for the given index.
    QString channelPrefix(int index) const;
    /// @return Full parameter name (e.g., "L_RCVR1") for a channel/receiver pair.
    QString parameterName(int channel_index, int receiver_index) const;
    /// @return Auto-generated timestamped output CSV filename.
    QString generateOutputFilename() const;

    /// Validates and parses day/hour/minute/second time field strings.
    bool validateTimeFields(const QString& ddd, const QString& hh,
                             const QString& mm, const QString& ss,
                             int& out_ddd, int& out_hh, int& out_mm, int& out_ss) const;

    /// Pre-validates time range strings. Returns empty on success, or a warning message.
    QString validateTimeRange(const QString& start_text, const QString& stop_text) const;
    /// @}

    /// @return Human-readable metadata summary for the status bar.
    QString fileMetadataSummary() const;

    /// @name Recent files
    /// @{
    QStringList recentFiles() const;             ///< @return List of recent file paths.
    void addRecentFile(const QString& filepath); ///< Adds a file to the recent files list.
    void clearRecentFiles();                     ///< Clears the recent files list.
    /// @}

    /// @name Model accessors
    /// @{
    Chapter10Reader* reader() const;             ///< @return Pointer to the Chapter10Reader instance.
    FrameSetup* frameSetup() const;              ///< @return Pointer to the FrameSetup instance.
    QString appRoot() const;                     ///< @return Application root directory path.
    QString lastIniDir() const;                  ///< @return Last directory used in INI file dialogs.
    /// @}

public slots:
    /// Logs startup configuration to the log window.
    void logStartupInfo();
    /// Opens a .ch10 file and populates channel lists.
    void openFile(const QString& filename);
    /// Opens multiple .ch10 files for batch processing.
    void openFiles(const QStringList& filenames);

    /// Sets the resolved PCM channel index for a batch file.
    void setBatchFilePcmChannel(int fileIndex, int channelIndex);
    /// Sets the resolved time channel index for a batch file.
    void setBatchFileTimeChannel(int fileIndex, int channelIndex);

    /**
     * @brief Validates inputs and starts background AGC processing.
     * @param[in] output_file       Path to the CSV output file.
     * @param[in] start_ddd         Start day-of-year string.
     * @param[in] start_hh          Start hour string.
     * @param[in] start_mm          Start minute string.
     * @param[in] start_ss          Start second string.
     * @param[in] stop_ddd          Stop day-of-year string.
     * @param[in] stop_hh           Stop hour string.
     * @param[in] stop_mm           Stop minute string.
     * @param[in] stop_ss           Stop second string.
     * @param[in] sample_rate_index Sample rate combo box index.
     */
    void startProcessing(const QString& output_file,
                         const QString& start_ddd, const QString& start_hh,
                         const QString& start_mm, const QString& start_ss,
                         const QString& stop_ddd, const QString& stop_hh,
                         const QString& stop_mm, const QString& stop_ss,
                         int sample_rate_index);

    /**
     * @brief Validates inputs and starts batch background AGC processing.
     * @param[in] output_dir        Path to the output directory for CSV files.
     * @param[in] sample_rate_index Sample rate combo box index.
     */
    void startBatchProcessing(const QString& output_dir, int sample_rate_index);

    /// Loads settings from an INI file and applies them.
    void loadSettings(const QString& filename);
    /// Saves the current state to an INI file.
    void saveSettings(const QString& filename);

    /// Resets all state to defaults and closes the loaded file.
    void clearState();
    /// Requests cancellation of the current processing run.
    void cancelProcessing();

signals:
    void inputFilenameChanged();      ///< Emitted when the input file path changes.
    void channelListsChanged();       ///< Emitted when channel combo box lists are rebuilt.
    void timeChannelIndexChanged();   ///< Emitted when the selected time channel changes.
    void pcmChannelIndexChanged();    ///< Emitted when the selected PCM channel changes.
    void fileLoadedChanged();         ///< Emitted when the file-loaded state changes.
    void progressPercentChanged();    ///< Emitted when the processing progress updates.
    void processingChanged();         ///< Emitted when processing starts or stops.
    void controlsEnabledChanged();    ///< Emitted when the controls-enabled state changes.
    void fileTimesChanged();          ///< Emitted when start/stop file times are updated.
    void extractAllTimeChanged();     ///< Emitted when the extract-all-time flag changes.
    void sampleRateIndexChanged();    ///< Emitted when the sample rate index changes.
    void settingsChanged();           ///< Emitted when any settings property changes.
    void receiverLayoutChanged();     ///< Emitted when receiver count or channels per receiver changes.
    /// Emitted when a single receiver/channel checked state changes.
    void receiverCheckedChanged(int receiver_index, int channel_index, bool checked);

    /// Emitted when batch mode changes.
    void batchModeChanged();
    /// Emitted when the batch file list changes.
    void batchFilesChanged();
    /// Emitted when a single batch file's channel selection or status changes.
    void batchFileUpdated(int fileIndex);
    /// Emitted when processing moves to the next file in a batch.
    void batchFileProcessing(int file_index, int total);

    /// Emitted when the recent files list changes.
    void recentFilesChanged();
    /// Emitted when a validation or processing error occurs.
    void errorOccurred(const QString& message);
    /// Emitted when background processing finishes.
    void processingFinished(bool success, const QString& output_file);
    /// Emitted when the background processor sends a log message.
    void logMessageReceived(const QString& message);

private:
    /// @brief Validated parameters bundle passed to the worker thread.
    struct ProcessingParams {
        QString filename;              ///< Path to the .ch10 input file.
        int time_channel_id;           ///< Resolved time channel ID.
        int pcm_channel_id;            ///< Resolved PCM channel ID.
        uint64_t frame_sync;           ///< Frame sync pattern as a numeric value.
        int sync_pattern_length;       ///< Sync pattern length in bits.
        int words_in_minor_frame;      ///< Words per PCM minor frame (data words + 1).
        int bits_in_minor_frame;       ///< Total bits per PCM minor frame.
        double scale_lower_bound;      ///< Lower dB bound (voltage_lower * range_dB_per_V).
        double scale_upper_bound;      ///< Upper dB bound (voltage_upper * range_dB_per_V).
        bool negative_polarity;        ///< True if AGC polarity is negative.
        uint64_t start_seconds;        ///< Start of extraction window (IRIG seconds).
        uint64_t stop_seconds;         ///< End of extraction window (IRIG seconds).
        int sample_rate;               ///< Output sample rate in Hz.
        QString outfile;               ///< Path to the CSV output file.
    };

    /// Validates user inputs and populates @p params for processing.
    bool validateProcessingInputs(ProcessingParams& params,
                                   const QString& start_ddd, const QString& start_hh,
                                   const QString& start_mm, const QString& start_ss,
                                   const QString& stop_ddd, const QString& stop_hh,
                                   const QString& stop_mm, const QString& stop_ss,
                                   int sample_rate_index);

    /// Runs a pre-scan on the given PCM channel to detect encoding and verify sync.
    void runPreScan(int pcm_channel_id);

    /// Validates all batch files against current channel/settings selection.
    void validateBatchFiles();
    /// Runs pre-scan on all valid batch files.
    void preScanBatchFiles();
    /// Launches processing for the next non-skipped batch file.
    void processNextBatchFile();
    /// Builds a name-to-index map for O(1) parameter lookup in the frame setup.
    QMap<QString, int> buildParameterMap() const;

    /// Applies calibration slope/scale to each enabled frame parameter.
    bool prepareFrameSetupParameters(double scale_lower_bound,
                                      double scale_upper_bound,
                                      bool negative_polarity);

    /// Creates a FrameProcessor and starts it on a background thread.
    void launchWorkerThread(const ProcessingParams& params);

    /// Slot: updates m_progress_percent from the worker thread.
    void onProgressUpdated(int percent);
    /// Slot: handles worker completion and cleans up the thread.
    void onProcessingFinished(bool success);
    /// Slot: forwards a log message from the worker thread.
    void onLogMessage(const QString& message);

    Chapter10Reader* m_reader;               ///< Chapter 10 file reader instance.
    FrameSetup* m_frame_setup;               ///< Frame parameter definitions.
    SettingsManager* m_settings;             ///< Settings persistence manager.
    QThread* m_worker_thread;                ///< Background processing thread.
    FrameProcessor* m_current_processor;     ///< Pointer to active processor (for abort).

    QString m_app_root;                      ///< Application root directory.
    QString m_input_filename;                ///< Path to the loaded .ch10 file.
    QString m_last_output_file;              ///< Path to the last generated CSV file.
    QString m_last_ini_dir;                  ///< Last directory used in INI file dialogs.
    bool m_file_loaded;                      ///< True when a .ch10 file is loaded.
    int m_progress_percent;                  ///< Processing progress (0--100).
    bool m_processing;                       ///< True while background processing is running.

    int m_time_channel_index;                ///< Selected time channel combo box index.
    int m_pcm_channel_index;                 ///< Selected PCM channel combo box index.

    bool m_extract_all_time;                 ///< True to extract full time duration.
    int m_sample_rate_index;                 ///< Selected sample rate combo box index.

    QString m_settings_frame_sync;                ///< Frame sync hex pattern.
    int m_settings_polarity_idx;                  ///< Polarity combo box index (0=Positive, 1=Negative).
    int m_settings_slope_idx;                     ///< Voltage slope combo box index.
    QString m_settings_scale;                     ///< Calibration scale in dB per volt.
    int m_settings_receiver_count;                ///< Number of receivers.
    int m_settings_channels_per_rcvr;             ///< Channels per receiver.

    QVector<QVector<bool>> m_receiver_states; ///< 2D grid of receiver/channel checked states.
    QStringList m_recent_files;              ///< Most-recently-opened file paths.

    /// @name Batch processing state
    /// @{
    QVector<BatchFileInfo> m_batch_files;    ///< Loaded file list for batch mode.
    bool m_batch_mode;                       ///< True when multiple files are loaded.
    int m_batch_current_index;               ///< Index of file currently being processed.
    bool m_batch_cancelled;                  ///< True if the user cancelled batch processing.
    QString m_batch_output_dir;              ///< User-selected output directory for batch.
    int m_batch_success_count;               ///< Number of successfully processed files.
    int m_batch_skip_count;                  ///< Number of skipped files.
    int m_batch_error_count;                 ///< Number of files that failed during processing.

    int m_batch_sample_rate_index = 0;       ///< Sample rate index for current batch run.
    /// @}
};

#endif // MAINVIEWMODEL_H