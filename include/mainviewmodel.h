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
#include <QThread>
#include <QVector>

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
    Q_PROPERTY(QString frameSync READ frameSync WRITE setFrameSync NOTIFY configChanged)
    Q_PROPERTY(bool negativePolarity READ negativePolarity WRITE setNegativePolarity NOTIFY configChanged)
    Q_PROPERTY(int scaleIndex READ scaleIndex WRITE setScaleIndex NOTIFY configChanged)
    Q_PROPERTY(QString range READ range WRITE setRange NOTIFY configChanged)
    Q_PROPERTY(int receiverCount READ receiverCount WRITE setReceiverCount NOTIFY receiverLayoutChanged)
    Q_PROPERTY(int channelsPerReceiver READ channelsPerReceiver WRITE setChannelsPerReceiver NOTIFY receiverLayoutChanged)

public:
    explicit MainViewModel(QObject* parent = nullptr);
    ~MainViewModel();

    // --- Property getters ---
    QString inputFilename() const;
    QStringList timeChannelList() const;
    QStringList pcmChannelList() const;
    int timeChannelIndex() const;
    int pcmChannelIndex() const;
    bool fileLoaded() const;
    int progressPercent() const;
    bool processing() const;
    bool controlsEnabled() const;

    bool extractAllTime() const;
    int sampleRateIndex() const;

    int startDayOfYear() const;
    int startHour() const;
    int startMinute() const;
    int startSecond() const;
    int stopDayOfYear() const;
    int stopHour() const;
    int stopMinute() const;
    int stopSecond() const;

    QString frameSync() const;
    bool negativePolarity() const;
    int scaleIndex() const;
    QString range() const;
    int receiverCount() const;
    int channelsPerReceiver() const;

    // --- Property setters ---
    void setTimeChannelIndex(int index);
    void setPcmChannelIndex(int index);
    void setExtractAllTime(bool value);
    void setSampleRateIndex(int value);
    void setFrameSync(const QString& value);
    void setNegativePolarity(bool value);
    void setScaleIndex(int value);
    void setRange(const QString& value);
    void setReceiverCount(int value);
    void setChannelsPerReceiver(int value);

    // --- Receiver grid state ---
    bool receiverChecked(int receiver_index, int channel_index) const;
    void setReceiverChecked(int receiver_index, int channel_index, bool checked);
    void setAllReceiversChecked(bool checked);

    // --- Settings integration ---
    SettingsData getSettingsData() const;
    void applySettingsData(const SettingsData& data);
    void loadFrameSetupFrom(const QString& filename);
    void saveFrameSetupTo(QSettings& settings);

    // --- Static helpers ---
    static QString channelPrefix(int index);
    static QString parameterName(int channel_index, int receiver_index);
    static QString generateOutputFilename();

    // --- Model accessors ---
    Chapter10Reader* reader() const;
    FrameSetup* frameSetup() const;

    QString appRoot() const;

public slots:
    void openFile(const QString& filename);

    void startProcessing(const QString& output_file,
                         const QString& start_ddd, const QString& start_hh,
                         const QString& start_mm, const QString& start_ss,
                         const QString& stop_ddd, const QString& stop_hh,
                         const QString& stop_mm, const QString& stop_ss,
                         int sample_rate_index);

    void applyConfig(const QString& frame_sync, bool neg_polarity,
                     int scale_idx, const QString& range,
                     int receiver_count, int channels_per_rcvr);

    void loadSettings(const QString& filename);
    void saveSettings(const QString& filename);

    void clearState();

signals:
    void inputFilenameChanged();
    void channelListsChanged();
    void timeChannelIndexChanged();
    void pcmChannelIndexChanged();
    void fileLoadedChanged();
    void progressPercentChanged();
    void processingChanged();
    void controlsEnabledChanged();
    void fileTimesChanged();
    void extractAllTimeChanged();
    void sampleRateIndexChanged();
    void configChanged();
    void receiverLayoutChanged();
    void receiverCheckedChanged(int receiver_index, int channel_index, bool checked);

    void errorOccurred(const QString& message);
    void processingFinished(bool success, const QString& output_file);
    void logMessageReceived(const QString& message);

private:
    /// @brief Validated parameters bundle passed to the worker thread.
    struct ProcessingParams {
        QString filename;
        int time_channel_id;
        int pcm_channel_id;
        uint64_t frame_sync;
        int sync_pattern_length;
        double scale_lower_bound;
        double scale_upper_bound;
        bool negative_polarity;
        uint64_t start_seconds;
        uint64_t stop_seconds;
        int sample_rate;
        QString outfile;
    };

    bool validateProcessingInputs(ProcessingParams& params,
                                   const QString& start_ddd, const QString& start_hh,
                                   const QString& start_mm, const QString& start_ss,
                                   const QString& stop_ddd, const QString& stop_hh,
                                   const QString& stop_mm, const QString& stop_ss,
                                   int sample_rate_index);

    static bool validateTimeFields(const QString& ddd, const QString& hh,
                                    const QString& mm, const QString& ss,
                                    int& out_ddd, int& out_hh, int& out_mm, int& out_ss);

    bool prepareFrameSetupParameters(double scale_lower_bound,
                                      double scale_upper_bound,
                                      bool negative_polarity);

    void launchWorkerThread(const ProcessingParams& params);

    void onProgressUpdated(int percent);
    void onProcessingFinished(bool success);
    void onLogMessage(const QString& message);

    Chapter10Reader* m_reader;
    FrameSetup* m_frame_setup;
    SettingsManager* m_settings;
    QThread* m_worker_thread;

    QString m_app_root;
    QString m_input_filename;
    QString m_last_output_file;
    bool m_file_loaded;
    int m_progress_percent;
    bool m_processing;

    int m_time_channel_index;
    int m_pcm_channel_index;

    bool m_extract_all_time;
    int m_sample_rate_index;

    QString m_cfg_frame_sync;
    bool m_cfg_neg_polarity;
    int m_cfg_scale_idx;
    QString m_cfg_range;
    int m_cfg_receiver_count;
    int m_cfg_channels_per_rcvr;

    QVector<QVector<bool>> m_receiver_states;
};

#endif // MAINVIEWMODEL_H