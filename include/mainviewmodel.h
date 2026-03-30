/**
 * @file mainviewmodel.h
 * @brief ViewModel mediating between the View (MainView) and Model layer.
 */

#ifndef MAINVIEWMODEL_H
#define MAINVIEWMODEL_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVector>

#include "batchfileinfo.h"
#include "processingparams.h"
#include "settingsdata.h"
#include "timefields.h"

class Chapter10Reader;
class FrameSetup;
class ProcessingCoordinator;
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

    MainViewModel(const MainViewModel&) = delete;
    MainViewModel& operator=(const MainViewModel&) = delete;
    MainViewModel(MainViewModel&&) = delete;
    MainViewModel& operator=(MainViewModel&&) = delete;

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
    static QString generateBatchOutputFilename(const QString& input_filepath);
    /// @return Cached status summary for the file list tree header.
    QString batchStatusSummary() const;
    /// Rebuilds the cached batch status summary string.
    void rebuildBatchStatusSummary();
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
    static QString channelPrefix(int index);
    /// @return Full parameter name (e.g., "L_RCVR1") for a channel/receiver pair.
    static QString parameterName(int channel_index, int receiver_index);
    /// @return Auto-generated timestamped output CSV filename.
    QString generateOutputFilename() const;

    /// Validates and parses day/hour/minute/second time field strings.
    static bool validateTimeFields(const QString& ddd, const QString& hh,
                                    const QString& mm, const QString& ss,
                                    TimeFields& out);

    /// Pre-validates time range strings. Returns empty on success, or a warning message.
    static QString validateTimeRange(const QString& start_text, const QString& stop_text);
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

    /// Re-queues only failed batch files and re-runs processing.
    void retryFailedFiles();
    /// Moves a batch file from index @p from to index @p to and emits batchFilesChanged().
    void reorderBatchFile(int from, int to);

    /**
     * @brief Validates inputs and starts background AGC processing.
     * @param[in] output_file       Path to the CSV output file.
     * @param[in] start_time        Start time in "DDD:HH:MM:SS" format.
     * @param[in] stop_time         Stop time in "DDD:HH:MM:SS" format.
     * @param[in] sample_rate_index Sample rate combo box index.
     */
    void startProcessing(const QString& output_file,
                         const QString& start_time,
                         const QString& stop_time,
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

#ifdef QT_TESTLIB_LIB
    /// Appends a pre-built BatchFileInfo and enables batch mode. For unit testing only.
    void addBatchFileForTesting(const BatchFileInfo& info);
#endif

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
    /// Validates user inputs and populates @p params for processing.
    bool validateProcessingInputs(ProcessingParams& params,
                                   const QString& start_time,
                                   const QString& stop_time,
                                   int sample_rate_index);

    /// Validates all batch files against current channel/settings selection.
    void validateBatchFiles();

    Chapter10Reader*        m_reader;       ///< Chapter 10 file reader instance.
    FrameSetup*             m_frame_setup;  ///< Frame parameter definitions.
    SettingsManager*        m_settings;     ///< Settings persistence manager.
    ProcessingCoordinator*  m_coordinator;  ///< Owns worker thread and batch state machine.

    QString m_app_root;                      ///< Application root directory.
    QString m_input_filename;                ///< Path to the loaded .ch10 file.
    QString m_last_output_file;              ///< Path to the last generated CSV file.
    QString m_last_ini_dir;                  ///< Last directory used in INI file dialogs.
    bool m_file_loaded;                      ///< True when a .ch10 file is loaded.

    int m_time_channel_index;                ///< Selected time channel combo box index.
    int m_pcm_channel_index;                 ///< Selected PCM channel combo box index.

    bool m_extract_all_time;                 ///< True to extract full time duration.
    int m_sample_rate_index;                 ///< Selected sample rate combo box index.

    QString m_settings_frame_sync;           ///< Frame sync hex pattern.
    int m_settings_polarity_idx;             ///< Polarity combo box index (0=Positive, 1=Negative).
    int m_settings_slope_idx;               ///< Voltage slope combo box index.
    QString m_settings_scale;               ///< Calibration scale in dB per volt.
    int m_settings_receiver_count;           ///< Number of receivers.
    int m_settings_channels_per_rcvr;        ///< Channels per receiver.

    QVector<QVector<bool>> m_receiver_states; ///< 2D grid of receiver/channel checked states.
    QStringList m_recent_files;              ///< Most-recently-opened file paths.

    /// @name Batch state (shared with coordinator via pointer)
    /// @{
    QVector<BatchFileInfo> m_batch_files;    ///< Loaded file list for batch mode.
    bool m_batch_mode;                       ///< True when multiple files are loaded.
    QString m_batch_status_summary;          ///< Cached summary string ("N files loaded (X valid, Y skipped)").
    /// @}
};

#endif // MAINVIEWMODEL_H
