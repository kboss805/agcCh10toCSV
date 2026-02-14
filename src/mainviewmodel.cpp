/**
 * @file mainviewmodel.cpp
 * @brief Implementation of MainViewModel — state management and processing orchestration.
 */

#include "mainviewmodel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMap>

#include "chapter10reader.h"
#include "constants.h"
#include "framesetup.h"
#include "frameprocessor.h"
#include "settingsmanager.h"

MainViewModel::MainViewModel(QObject* parent)
    : QObject(parent),
      m_worker_thread(nullptr),
      m_current_processor(nullptr),
      m_file_loaded(false),
      m_progress_percent(0),
      m_processing(false),
      m_time_channel_index(0),
      m_pcm_channel_index(0),
      m_extract_all_time(true),
      m_sample_rate_index(0),
      m_settings_frame_sync(PCMConstants::kDefaultFrameSync),
      m_settings_polarity_idx(0),
      m_settings_slope_idx(UIConstants::kDefaultSlopeIndex),
      m_settings_scale(UIConstants::kDefaultScale),
      m_settings_receiver_count(UIConstants::kDefaultReceiverCount),
      m_settings_channels_per_rcvr(UIConstants::kDefaultChannelsPerReceiver)
{
    m_app_root = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    m_reader = new Chapter10Reader();
    m_frame_setup = new FrameSetup(this);
    m_settings = new SettingsManager(this);

    QSettings app_settings;
    m_last_ini_dir = app_settings.value(UIConstants::kSettingsKeyLastIniDir).toString();
    if (m_last_ini_dir.isEmpty())
        m_last_ini_dir = m_app_root + "/settings";

    QSettings config(m_app_root + "/settings/default.ini", QSettings::IniFormat);
    QString ini_sync = config.value("Frame/FrameSync").toString();
    if (!ini_sync.isEmpty())
        m_settings_frame_sync = ini_sync;
    m_settings_polarity_idx = config.value("Parameters/Polarity", 0).toInt();
    m_settings_slope_idx = config.value("Parameters/Slope", UIConstants::kDefaultSlopeIndex).toInt();
    QString ini_range = config.value("Parameters/Scale").toString();
    if (!ini_range.isEmpty())
        m_settings_scale = ini_range;
    m_settings_receiver_count = config.value("Receivers/Count", UIConstants::kDefaultReceiverCount).toInt();
    m_settings_channels_per_rcvr =
        config.value("Receivers/ChannelsPerReceiver", UIConstants::kDefaultChannelsPerReceiver).toInt();

    m_receiver_states.resize(m_settings_receiver_count);
    for (int i = 0; i < m_settings_receiver_count; i++)
        m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);

    loadFrameSetupFrom(m_app_root + "/settings/default.ini");

    connect(m_reader, &Chapter10Reader::displayErrorMessage,
            this, &MainViewModel::errorOccurred);
    connect(m_settings, &SettingsManager::logMessage,
            this, &MainViewModel::logMessageReceived);
}

MainViewModel::~MainViewModel()
{
    if (m_worker_thread && m_worker_thread->isRunning())
    {
        if (m_current_processor)
            m_current_processor->requestAbort();
        m_worker_thread->quit();
        m_worker_thread->wait();
    }

    delete m_reader;
    delete m_frame_setup;
    delete m_settings;
}

////////////////////////////////////////////////////////////////////////////////
//                            PROPERTY GETTERS                                //
////////////////////////////////////////////////////////////////////////////////

QString MainViewModel::inputFilename() const { return m_input_filename; }
QStringList MainViewModel::timeChannelList() const { return m_reader->getTimeChannelComboBoxList(); }
QStringList MainViewModel::pcmChannelList() const { return m_reader->getPCMChannelComboBoxList(); }
int MainViewModel::timeChannelIndex() const { return m_time_channel_index; }
int MainViewModel::pcmChannelIndex() const { return m_pcm_channel_index; }
bool MainViewModel::fileLoaded() const { return m_file_loaded; }
int MainViewModel::progressPercent() const { return m_progress_percent; }
bool MainViewModel::processing() const { return m_processing; }
bool MainViewModel::controlsEnabled() const { return m_file_loaded && !m_processing; }

bool MainViewModel::extractAllTime() const { return m_extract_all_time; }
int MainViewModel::sampleRateIndex() const { return m_sample_rate_index; }

int MainViewModel::startDayOfYear() const { return m_reader->getStartDayOfYear(); }
int MainViewModel::startHour() const { return m_reader->getStartHour(); }
int MainViewModel::startMinute() const { return m_reader->getStartMinute(); }
int MainViewModel::startSecond() const { return m_reader->getStartSecond(); }
int MainViewModel::stopDayOfYear() const { return m_reader->getStopDayOfYear(); }
int MainViewModel::stopHour() const { return m_reader->getStopHour(); }
int MainViewModel::stopMinute() const { return m_reader->getStopMinute(); }
int MainViewModel::stopSecond() const { return m_reader->getStopSecond(); }

QString MainViewModel::frameSync() const { return m_settings_frame_sync; }
int MainViewModel::polarityIndex() const { return m_settings_polarity_idx; }
int MainViewModel::slopeIndex() const { return m_settings_slope_idx; }
QString MainViewModel::scale() const { return m_settings_scale; }
int MainViewModel::receiverCount() const { return m_settings_receiver_count; }
int MainViewModel::channelsPerReceiver() const { return m_settings_channels_per_rcvr; }

////////////////////////////////////////////////////////////////////////////////
//                            PROPERTY SETTERS                                //
////////////////////////////////////////////////////////////////////////////////

void MainViewModel::setTimeChannelIndex(int index)
{
    if (m_time_channel_index == index)
        return;
    m_time_channel_index = index;
    // Subtract 1 to account for "Select a..." placeholder item
    m_reader->timeChannelChanged(index);
    emit timeChannelIndexChanged();
}

void MainViewModel::setPcmChannelIndex(int index)
{
    if (m_pcm_channel_index == index)
        return;
    m_pcm_channel_index = index;
    m_reader->pcmChannelChanged(index);
    emit pcmChannelIndexChanged();

    if (m_file_loaded)
        runPreScan(m_reader->getCurrentPCMChannelID());
}

void MainViewModel::setExtractAllTime(bool value)
{
    if (m_extract_all_time == value)
        return;
    m_extract_all_time = value;
    emit extractAllTimeChanged();
}

void MainViewModel::setSampleRateIndex(int value)
{
    if (m_sample_rate_index == value)
        return;
    m_sample_rate_index = value;
    emit sampleRateIndexChanged();
}

void MainViewModel::setFrameSync(const QString& value)
{
    if (m_settings_frame_sync == value)
        return;
    m_settings_frame_sync = value;
    emit settingsChanged();
}

void MainViewModel::setPolarityIndex(int value)
{
    if (m_settings_polarity_idx == value)
        return;
    m_settings_polarity_idx = value;
    emit settingsChanged();
}

void MainViewModel::setSlopeIndex(int value)
{
    if (m_settings_slope_idx == value)
        return;
    m_settings_slope_idx = value;
    emit settingsChanged();
}

void MainViewModel::setScale(const QString& value)
{
    if (m_settings_scale == value)
        return;
    m_settings_scale = value;
    emit settingsChanged();
}

void MainViewModel::setReceiverCount(int value)
{
    if (m_settings_receiver_count == value)
        return;
    m_settings_receiver_count = value;
    m_receiver_states.resize(value);
    for (int i = 0; i < value; i++)
        if (m_receiver_states[i].size() != m_settings_channels_per_rcvr)
            m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
    emit receiverLayoutChanged();
}

void MainViewModel::setChannelsPerReceiver(int value)
{
    if (m_settings_channels_per_rcvr == value)
        return;
    m_settings_channels_per_rcvr = value;
    for (int i = 0; i < m_receiver_states.size(); i++)
        m_receiver_states[i].fill(true, value);
    emit receiverLayoutChanged();
}

////////////////////////////////////////////////////////////////////////////////
//                          RECEIVER GRID STATE                               //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::receiverChecked(int receiver_index, int channel_index) const
{
    if (receiver_index < 0 || receiver_index >= m_receiver_states.size())
        return false;
    if (channel_index < 0 || channel_index >= m_receiver_states[receiver_index].size())
        return false;
    return m_receiver_states[receiver_index][channel_index];
}

void MainViewModel::setReceiverChecked(int receiver_index, int channel_index, bool checked)
{
    if (receiver_index < 0 || receiver_index >= m_receiver_states.size())
        return;
    if (channel_index < 0 || channel_index >= m_receiver_states[receiver_index].size())
        return;
    if (m_receiver_states[receiver_index][channel_index] == checked)
        return;
    m_receiver_states[receiver_index][channel_index] = checked;
    emit receiverCheckedChanged(receiver_index, channel_index, checked);
}

void MainViewModel::setAllReceiversChecked(bool checked)
{
    for (int r = 0; r < m_receiver_states.size(); r++)
        for (int c = 0; c < m_receiver_states[r].size(); c++)
            m_receiver_states[r][c] = checked;
}

////////////////////////////////////////////////////////////////////////////////
//                         SETTINGS INTEGRATION                               //
////////////////////////////////////////////////////////////////////////////////

SettingsData MainViewModel::getSettingsData() const
{
    SettingsData data;
    data.frameSync = m_settings_frame_sync;
    data.polarityIndex = m_settings_polarity_idx;
    data.slopeIndex = m_settings_slope_idx;
    data.scale = m_settings_scale;
    data.extractAllTime = m_extract_all_time;
    data.sampleRateIndex = m_sample_rate_index;
    data.receiverCount = m_settings_receiver_count;
    data.channelsPerReceiver = m_settings_channels_per_rcvr;
    return data;
}

void MainViewModel::applySettingsData(const SettingsData& data)
{
    m_settings_frame_sync = data.frameSync;
    m_settings_polarity_idx = data.polarityIndex;
    m_settings_slope_idx = data.slopeIndex;
    m_settings_scale = data.scale;

    int old_receiver_count = m_settings_receiver_count;
    int old_channel_count = m_settings_channels_per_rcvr;
    m_settings_receiver_count = data.receiverCount;
    m_settings_channels_per_rcvr = data.channelsPerReceiver;

    m_extract_all_time = data.extractAllTime;
    m_sample_rate_index = data.sampleRateIndex;

    if (m_settings_receiver_count != old_receiver_count ||
        m_settings_channels_per_rcvr != old_channel_count)
    {
        m_receiver_states.resize(m_settings_receiver_count);
        for (int i = 0; i < m_settings_receiver_count; i++)
            m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
        emit receiverLayoutChanged();
    }

    emit extractAllTimeChanged();
    emit sampleRateIndexChanged();
    emit settingsChanged();
}

void MainViewModel::loadFrameSetupFrom(const QString& filename)
{
    int words_in_frame = m_settings_receiver_count * m_settings_channels_per_rcvr + 1;
    m_frame_setup->tryLoadingFile(filename, words_in_frame);
}

void MainViewModel::saveFrameSetupTo(QSettings& settings)
{
    QMap<QString, int> param_map = buildParameterMap();

    // Sync receiver states to parameter Enabled before saving
    for (int receiver_index = 0; receiver_index < m_receiver_states.size(); receiver_index++)
    {
        for (int channel_index = 0;
             channel_index < m_receiver_states[receiver_index].size();
             channel_index++)
        {
            bool checked = m_receiver_states[receiver_index][channel_index];
            QString name = parameterName(channel_index, receiver_index);
            auto it = param_map.constFind(name);
            if (it != param_map.constEnd())
                m_frame_setup->getParameter(it.value())->is_enabled = checked;
        }
    }

    m_frame_setup->saveToSettings(settings);
}

////////////////////////////////////////////////////////////////////////////////
//                               HELPERS                                      //
////////////////////////////////////////////////////////////////////////////////

void MainViewModel::runPreScan(int pcm_channel_id)
{
    if (pcm_channel_id < 0 || m_settings_frame_sync.isEmpty() || m_frame_setup->length() == 0)
        return;

    bool sync_ok = false;
    uint64_t scan_sync = m_settings_frame_sync.toULongLong(&sync_ok, 16);
    if (!sync_ok)
        return;

    int scan_sync_len = m_settings_frame_sync.length() * 4;
    int data_words = m_frame_setup->length();
    int scan_words = data_words + 1;
    int scan_bits = data_words * PCMConstants::kCommonWordLen + scan_sync_len;

    FrameProcessor scanner;
    connect(&scanner, &FrameProcessor::logMessage,
            this, &MainViewModel::logMessageReceived);

    scanner.preScan(m_input_filename, pcm_channel_id, scan_sync,
                    scan_sync_len, scan_words, scan_bits);
}

QMap<QString, int> MainViewModel::buildParameterMap() const
{
    QMap<QString, int> param_map;
    for (int i = 0; i < m_frame_setup->length(); i++)
        param_map.insert(m_frame_setup->getParameter(i)->name, i);
    return param_map;
}

QString MainViewModel::channelPrefix(int index) const
{
    if (index < UIConstants::kNumKnownPrefixes)
        return UIConstants::kChannelPrefixes[index];
    return "CH" + QString::number(index + 1);
}

QString MainViewModel::parameterName(int channel_index, int receiver_index) const
{
    return channelPrefix(channel_index) + "_RCVR" + QString::number(receiver_index + 1);
}

QString MainViewModel::generateOutputFilename() const
{
    return QString(UIConstants::kOutputPrefix) +
           QDateTime::currentDateTime().toString(UIConstants::kOutputTimestampFormat) +
           UIConstants::kOutputExtension;
}

////////////////////////////////////////////////////////////////////////////////
//                            MODEL ACCESSORS                                 //
////////////////////////////////////////////////////////////////////////////////

Chapter10Reader* MainViewModel::reader() const { return m_reader; }
FrameSetup* MainViewModel::frameSetup() const { return m_frame_setup; }
QString MainViewModel::appRoot() const { return m_app_root; }
QString MainViewModel::lastIniDir() const { return m_last_ini_dir; }

////////////////////////////////////////////////////////////////////////////////
//                              COMMANDS                                      //
////////////////////////////////////////////////////////////////////////////////

void MainViewModel::logStartupInfo()
{
    emit logMessageReceived("agcCh10toCSV v" + AppVersion::toString());
    emit logMessageReceived("Startup settings loaded from default.ini");
    emit logMessageReceived("  FrameSync=" + m_settings_frame_sync +
        ", Polarity=" + QString(UIConstants::kPolarityLabels[m_settings_polarity_idx]) +
        ", Slope=" + QString(UIConstants::kSlopeLabels[m_settings_slope_idx]) +
        ", Scale=" + m_settings_scale + " dB/V");
    emit logMessageReceived("  Receivers=" + QString::number(m_settings_receiver_count) +
        ", Channels=" + QString::number(m_settings_channels_per_rcvr) +
        ", Frame setup=" + QString::number(m_frame_setup->length()) + " parameters");
}

void MainViewModel::openFile(const QString& filename)
{
    clearState();

    m_input_filename = filename;
    QFileInfo file_info(filename);
    emit logMessageReceived("Opening: " + file_info.fileName());

    if (!m_reader->loadChannels(filename))
    {
        m_input_filename.clear();
        return;
    }

    // Log file metadata
    qint64 file_bytes = file_info.size();
    QString size_str = (file_bytes >= 1024 * 1024)
        ? QString::number(file_bytes / (1024.0 * 1024.0), 'f', 1) + " MB"
        : QString::number(file_bytes / 1024.0, 'f', 1) + " KB";
    int duration_sec = (m_reader->getStopDayOfYear() - m_reader->getStartDayOfYear()) * 86400
        + (m_reader->getStopHour() - m_reader->getStartHour()) * 3600
        + (m_reader->getStopMinute() - m_reader->getStartMinute()) * 60
        + (m_reader->getStopSecond() - m_reader->getStartSecond());
    emit logMessageReceived("  File size: " + size_str +
        ", Recording duration: " + QString::number(duration_sec) + "s");

    // Log channels found
    QStringList time_list = m_reader->getTimeChannelComboBoxList();
    QStringList pcm_list = m_reader->getPCMChannelComboBoxList();
    emit logMessageReceived("  Time channels: " + (time_list.isEmpty() ? "none" : QString::number(time_list.size())));
    for (const QString& ch : time_list)
        emit logMessageReceived("    " + ch);
    emit logMessageReceived("  PCM channels: " + (pcm_list.isEmpty() ? "none" : QString::number(pcm_list.size())));
    for (const QString& ch : pcm_list)
        emit logMessageReceived("    " + ch);

    // Log file time range
    emit logMessageReceived("  Time range: " +
        QString("%1:%2:%3:%4")
            .arg(m_reader->getStartDayOfYear(), 3, 10, QChar('0'))
            .arg(m_reader->getStartHour(), 2, 10, QChar('0'))
            .arg(m_reader->getStartMinute(), 2, 10, QChar('0'))
            .arg(m_reader->getStartSecond(), 2, 10, QChar('0')) +
        " - " +
        QString("%1:%2:%3:%4")
            .arg(m_reader->getStopDayOfYear(), 3, 10, QChar('0'))
            .arg(m_reader->getStopHour(), 2, 10, QChar('0'))
            .arg(m_reader->getStopMinute(), 2, 10, QChar('0'))
            .arg(m_reader->getStopSecond(), 2, 10, QChar('0')));

    // Log current frame settings
    emit logMessageReceived("  FrameSync=" + m_settings_frame_sync +
        ", Polarity=" + QString(UIConstants::kPolarityLabels[m_settings_polarity_idx]) +
        ", Slope=" + QString(UIConstants::kSlopeLabels[m_settings_slope_idx]) +
        ", Scale=" + m_settings_scale + " dB/V");
    emit logMessageReceived("  Receivers=" + QString::number(m_settings_receiver_count) +
        ", Channels=" + QString::number(m_settings_channels_per_rcvr) +
        ", Frame setup=" + QString::number(m_frame_setup->length()) + " parameters");

    m_file_loaded = true;
    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileTimesChanged();
    emit fileLoadedChanged();
}

void MainViewModel::startProcessing(const QString& output_file,
                                     const QString& start_ddd, const QString& start_hh,
                                     const QString& start_mm, const QString& start_ss,
                                     const QString& stop_ddd, const QString& stop_hh,
                                     const QString& stop_mm, const QString& stop_ss,
                                     int sample_rate_index)
{
    ProcessingParams params;
    if (!validateProcessingInputs(params, start_ddd, start_hh, start_mm, start_ss,
                                   stop_ddd, stop_hh, stop_mm, stop_ss,
                                   sample_rate_index))
        return;

    if (!prepareFrameSetupParameters(params.scale_lower_bound,
                                      params.scale_upper_bound,
                                      params.negative_polarity))
    {
        emit errorOccurred("No receivers selected.");
        return;
    }

    params.outfile = output_file;
    m_last_output_file = output_file;

    // Guard against re-entry
    if (m_worker_thread && m_worker_thread->isRunning())
        return;

    m_processing = true;
    m_progress_percent = 0;
    emit processingChanged();
    emit controlsEnabledChanged();
    emit progressPercentChanged();

    launchWorkerThread(params);
}

void MainViewModel::loadSettings(const QString& filename)
{
    m_settings->loadFile(filename);
    m_last_ini_dir = QFileInfo(filename).absolutePath();
    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyLastIniDir, m_last_ini_dir);
}

void MainViewModel::saveSettings(const QString& filename)
{
    m_settings->saveFile(filename);
    m_last_ini_dir = QFileInfo(filename).absolutePath();
    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyLastIniDir, m_last_ini_dir);
}

void MainViewModel::clearState()
{
    m_input_filename.clear();
    m_last_output_file.clear();
    m_file_loaded = false;
    m_progress_percent = 0;
    m_processing = false;
    m_time_channel_index = 0;
    m_pcm_channel_index = 0;

    m_reader->clearSettings();

    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileLoadedChanged();
    emit fileTimesChanged();
    emit progressPercentChanged();
    emit processingChanged();
    emit controlsEnabledChanged();
}

void MainViewModel::cancelProcessing()
{
    if (m_current_processor)
        m_current_processor->requestAbort();
}

////////////////////////////////////////////////////////////////////////////////
//                        VALIDATION & PROCESSING                             //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::validateTimeFields(const QString& ddd, const QString& hh,
                                        const QString& mm, const QString& ss,
                                        int& out_ddd, int& out_hh, int& out_mm, int& out_ss) const
{
    bool ddd_ok, hh_ok, mm_ok, ss_ok;
    out_ddd = ddd.toInt(&ddd_ok);
    out_hh = hh.toInt(&hh_ok);
    out_mm = mm.toInt(&mm_ok);
    out_ss = ss.toInt(&ss_ok);

    return ddd_ok && hh_ok && mm_ok && ss_ok &&
           out_ddd >= UIConstants::kMinDayOfYear && out_ddd <= UIConstants::kMaxDayOfYear &&
           out_hh >= 0 && out_hh <= UIConstants::kMaxHour &&
           out_mm >= 0 && out_mm <= UIConstants::kMaxMinute &&
           out_ss >= 0 && out_ss <= UIConstants::kMaxSecond;
}

QString MainViewModel::validateTimeRange(const QString& start_text,
                                          const QString& stop_text) const
{
    QStringList start_parts = start_text.split(":");
    QStringList stop_parts = stop_text.split(":");

    if (start_parts.size() != 4 || stop_parts.size() != 4)
        return "Start and stop times must be in DDD:HH:MM:SS format.";

    int s_ddd, s_hh, s_mm, s_ss;
    if (!validateTimeFields(start_parts[0], start_parts[1],
                             start_parts[2], start_parts[3],
                             s_ddd, s_hh, s_mm, s_ss))
        return "Start time is out of valid range. Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.";

    int e_ddd, e_hh, e_mm, e_ss;
    if (!validateTimeFields(stop_parts[0], stop_parts[1],
                             stop_parts[2], stop_parts[3],
                             e_ddd, e_hh, e_mm, e_ss))
        return "Stop time is out of valid range. Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.";

    long long start_total = s_ddd * (long long)UIConstants::kSecondsPerDay
        + s_hh * (long long)UIConstants::kSecondsPerHour
        + s_mm * (long long)UIConstants::kSecondsPerMinute + s_ss;
    long long stop_total = e_ddd * (long long)UIConstants::kSecondsPerDay
        + e_hh * (long long)UIConstants::kSecondsPerHour
        + e_mm * (long long)UIConstants::kSecondsPerMinute + e_ss;

    if (stop_total <= start_total)
        return "Stop time must be after start time.";

    return {};
}

bool MainViewModel::validateProcessingInputs(ProcessingParams& params,
                                              const QString& start_ddd, const QString& start_hh,
                                              const QString& start_mm, const QString& start_ss,
                                              const QString& stop_ddd, const QString& stop_hh,
                                              const QString& stop_mm, const QString& stop_ss,
                                              int sample_rate_index)
{
    params.filename = m_input_filename;
    if (params.filename.isEmpty())
    {
        emit errorOccurred("No file loaded.");
        return false;
    }

    params.time_channel_id = m_reader->getCurrentTimeChannelID();
    if (params.time_channel_id < 0)
    {
        emit errorOccurred("Invalid time channel.");
        return false;
    }

    params.pcm_channel_id = m_reader->getCurrentPCMChannelID();
    if (params.pcm_channel_id < 0)
    {
        emit errorOccurred("Invalid PCM channel.");
        return false;
    }

    bool frame_sync_ok;
    params.frame_sync = m_settings_frame_sync.toULongLong(&frame_sync_ok, 16);
    if (!frame_sync_ok)
    {
        emit errorOccurred("Invalid frame sync.");
        return false;
    }

    params.sync_pattern_length = m_settings_frame_sync.length() * 4;
    if (params.sync_pattern_length <= 0)
    {
        emit errorOccurred("Frame sync pattern is empty.");
        return false;
    }

    int data_words = m_frame_setup->length();
    params.words_in_minor_frame = data_words + 1;
    params.bits_in_minor_frame = data_words * PCMConstants::kCommonWordLen + params.sync_pattern_length;

    if (params.sync_pattern_length > params.bits_in_minor_frame)
    {
        emit errorOccurred("Frame sync pattern (" +
                            QString::number(params.sync_pattern_length) +
                            " bits) exceeds frame length (" +
                            QString::number(params.bits_in_minor_frame) + " bits).");
        return false;
    }

    if (m_frame_setup->length() == 0)
    {
        emit errorOccurred("Frame setup not loaded.");
        return false;
    }

    double scale_dB_per_V = m_settings_scale.toDouble();
    if (scale_dB_per_V <= 0)
    {
        emit errorOccurred("Scale must be a positive number.");
        return false;
    }

    int scale_index = m_settings_slope_idx;
    if (scale_index < 0 || scale_index > UIConstants::kMaxSlopeIndex)
    {
        emit errorOccurred("Invalid slope index.");
        return false;
    }

    double voltage_lower = UIConstants::kSlopeVoltageLower[scale_index];
    double voltage_upper = UIConstants::kSlopeVoltageUpper[scale_index];
    params.scale_lower_bound = voltage_lower * scale_dB_per_V;
    params.scale_upper_bound = voltage_upper * scale_dB_per_V;

    params.negative_polarity = (m_settings_polarity_idx == 1);

    int s_ddd, s_hh, s_mm, s_ss;
    if (!validateTimeFields(start_ddd, start_hh, start_mm, start_ss,
                             s_ddd, s_hh, s_mm, s_ss))
    {
        emit errorOccurred("Invalid start time.");
        return false;
    }
    params.start_seconds = m_reader->dhmsToUInt64(s_ddd, s_hh, s_mm, s_ss);

    int e_ddd, e_hh, e_mm, e_ss;
    if (!validateTimeFields(stop_ddd, stop_hh, stop_mm, stop_ss,
                             e_ddd, e_hh, e_mm, e_ss))
    {
        emit errorOccurred("Invalid stop time.");
        return false;
    }
    params.stop_seconds = m_reader->dhmsToUInt64(e_ddd, e_hh, e_mm, e_ss);

    if (params.stop_seconds < params.start_seconds)
    {
        emit errorOccurred("Stop time must be after start time.");
        return false;
    }

    switch (sample_rate_index)
    {
        case 0: params.sample_rate = UIConstants::kSampleRate1Hz; break;
        case 1: params.sample_rate = UIConstants::kSampleRate10Hz; break;
        case 2: params.sample_rate = UIConstants::kSampleRate100Hz; break;
        default:
            emit errorOccurred("Invalid sample rate.");
            return false;
    }

    return true;
}

bool MainViewModel::prepareFrameSetupParameters(double scale_lower_bound,
                                                  double scale_upper_bound,
                                                  bool negative_polarity)
{
    bool any_enabled = false;

    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        ParameterInfo* param = m_frame_setup->getParameter(i);
        param->slope = (scale_upper_bound - scale_lower_bound) / PCMConstants::kMaxRawSampleValue;
        if (negative_polarity)
        {
            param->slope *= -1;
            param->scale = -scale_upper_bound / (scale_upper_bound - scale_lower_bound) * PCMConstants::kMaxRawSampleValue;
        }
        else
        {
            param->scale = scale_lower_bound / (scale_upper_bound - scale_lower_bound) * PCMConstants::kMaxRawSampleValue;
        }
        param->is_enabled = false;
    }

    // Enable only parameters that correspond to checked receivers in the grid
    QMap<QString, int> param_map = buildParameterMap();

    for (int receiver_index = 0; receiver_index < m_receiver_states.size(); receiver_index++)
    {
        for (int channel_index = 0;
             channel_index < m_receiver_states[receiver_index].size();
             channel_index++)
        {
            if (m_receiver_states[receiver_index][channel_index])
            {
                QString name = parameterName(channel_index, receiver_index);
                auto it = param_map.constFind(name);
                if (it != param_map.constEnd())
                {
                    m_frame_setup->getParameter(it.value())->is_enabled = true;
                    any_enabled = true;
                }
            }
        }
    }

    return any_enabled;
}

void MainViewModel::launchWorkerThread(const ProcessingParams& params)
{
    if (m_worker_thread)
    {
        m_worker_thread->quit();
        m_worker_thread->wait();
        delete m_worker_thread;
        m_worker_thread = nullptr;
    }

    m_worker_thread = new QThread;

    auto* processor = new FrameProcessor;
    m_current_processor = processor;
    processor->moveToThread(m_worker_thread);

    connect(processor, &FrameProcessor::progressUpdated,
            this, &MainViewModel::onProgressUpdated);
    connect(processor, &FrameProcessor::processingFinished,
            this, &MainViewModel::onProcessingFinished);
    connect(processor, &FrameProcessor::logMessage,
            this, &MainViewModel::onLogMessage);
    connect(processor, &FrameProcessor::errorOccurred,
            this, &MainViewModel::errorOccurred);

    connect(m_worker_thread, &QThread::finished,
            processor, &QObject::deleteLater);

    connect(m_worker_thread, &QThread::started, processor, [=]() {
        processor->process(params.filename, m_frame_setup, params.outfile,
                           params.time_channel_id, params.pcm_channel_id,
                           params.frame_sync, params.sync_pattern_length,
                           params.words_in_minor_frame, params.bits_in_minor_frame,
                           params.start_seconds, params.stop_seconds, params.sample_rate);
    });

    m_worker_thread->start();
}

void MainViewModel::onProgressUpdated(int percent)
{
    m_progress_percent = percent;
    emit progressPercentChanged();
}

void MainViewModel::onProcessingFinished(bool success)
{
    m_current_processor = nullptr;

    m_worker_thread->quit();
    m_worker_thread->wait();

    m_worker_thread->deleteLater();
    m_worker_thread = nullptr;

    if (success)
        m_progress_percent = 100;

    m_processing = false;
    emit progressPercentChanged();
    emit processingChanged();
    emit controlsEnabledChanged();
    emit processingFinished(success, m_last_output_file);
}

void MainViewModel::onLogMessage(const QString& message)
{
    emit logMessageReceived(message);
}
