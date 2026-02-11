/**
 * @file mainviewmodel.cpp
 * @brief Implementation of MainViewModel — state management and processing orchestration.
 */

#include "mainviewmodel.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QMap>

#include "chapter10reader.h"
#include "constants.h"
#include "framesetup.h"
#include "frameprocessor.h"
#include "settingsmanager.h"

MainViewModel::MainViewModel(QObject* parent)
    : QObject(parent),
      m_worker_thread(nullptr),
      m_file_loaded(false),
      m_progress_percent(0),
      m_processing(false),
      m_time_channel_index(0),
      m_pcm_channel_index(0),
      m_extract_all_time(true),
      m_sample_rate_index(0),
      m_settings_neg_polarity(false),
      m_settings_scale_idx(UIConstants::kDefaultScaleIndex),
      m_settings_range(UIConstants::kDefaultRange),
      m_settings_receiver_count(UIConstants::kDefaultReceiverCount),
      m_settings_channels_per_rcvr(UIConstants::kDefaultChannelsPerReceiver)
{
    m_app_root = QCoreApplication::applicationDirPath() + "/..";
    m_reader = new Chapter10Reader();
    m_frame_setup = new FrameSetup(this);
    m_settings = new SettingsManager(this);

    m_receiver_states.resize(m_settings_receiver_count);
    for (int i = 0; i < m_settings_receiver_count; i++)
        m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);

    connect(m_reader, &Chapter10Reader::displayErrorMessage,
            this, &MainViewModel::errorOccurred);
}

MainViewModel::~MainViewModel()
{
    if (m_worker_thread && m_worker_thread->isRunning())
    {
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
bool MainViewModel::negativePolarity() const { return m_settings_neg_polarity; }
int MainViewModel::scaleIndex() const { return m_settings_scale_idx; }
QString MainViewModel::range() const { return m_settings_range; }
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

void MainViewModel::setNegativePolarity(bool value)
{
    if (m_settings_neg_polarity == value)
        return;
    m_settings_neg_polarity = value;
    emit settingsChanged();
}

void MainViewModel::setScaleIndex(int value)
{
    if (m_settings_scale_idx == value)
        return;
    m_settings_scale_idx = value;
    emit settingsChanged();
}

void MainViewModel::setRange(const QString& value)
{
    if (m_settings_range == value)
        return;
    m_settings_range = value;
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
    data.timeChannelId = m_reader->getCurrentTimeChannelID();
    data.pcmChannelId = m_reader->getCurrentPCMChannelID();
    data.frameSync = m_settings_frame_sync;
    data.negativePolarity = m_settings_neg_polarity;
    data.scaleIndex = m_settings_scale_idx;
    data.range = m_settings_range;
    data.extractAllTime = m_extract_all_time;
    data.sampleRateIndex = m_sample_rate_index;
    data.receiverCount = m_settings_receiver_count;
    data.channelsPerReceiver = m_settings_channels_per_rcvr;
    return data;
}

void MainViewModel::applySettingsData(const SettingsData& data)
{
    m_settings_frame_sync = data.frameSync;
    m_settings_neg_polarity = data.negativePolarity;
    m_settings_scale_idx = data.scaleIndex;
    m_settings_range = data.range;

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
    m_frame_setup->tryLoadingFile(filename, PCMConstants::kWordsInMinorFrame);

    if (m_frame_setup->length() == 0)
        return;

    int expected_count = m_settings_receiver_count * m_settings_channels_per_rcvr;
    if (m_frame_setup->length() != expected_count)
    {
        qWarning() << "Frame setup has" << m_frame_setup->length()
                    << "parameters but expected" << expected_count
                    << "(" << m_settings_receiver_count << "receivers x"
                    << m_settings_channels_per_rcvr << "channels)";
    }

    // Build name -> index map for O(1) parameter lookup
    QMap<QString, int> param_map;
    for (int j = 0; j < m_frame_setup->length(); j++)
        param_map.insert(m_frame_setup->getParameter(j)->name, j);

    // Sync receiver states from loaded parameter Enabled states
    for (int receiver_index = 0; receiver_index < m_receiver_states.size(); receiver_index++)
    {
        for (int channel_index = 0;
             channel_index < m_receiver_states[receiver_index].size();
             channel_index++)
        {
            QString name = parameterName(channel_index, receiver_index);
            bool enabled = true;
            auto it = param_map.constFind(name);
            if (it != param_map.constEnd())
                enabled = m_frame_setup->getParameter(it.value())->is_enabled;
            m_receiver_states[receiver_index][channel_index] = enabled;
            emit receiverCheckedChanged(receiver_index, channel_index, enabled);
        }
    }
}

void MainViewModel::saveFrameSetupTo(QSettings& settings)
{
    // Build name -> index map for O(1) parameter lookup
    QMap<QString, int> param_map;
    for (int j = 0; j < m_frame_setup->length(); j++)
        param_map.insert(m_frame_setup->getParameter(j)->name, j);

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

////////////////////////////////////////////////////////////////////////////////
//                              COMMANDS                                      //
////////////////////////////////////////////////////////////////////////////////

void MainViewModel::openFile(const QString& filename)
{
    clearState();

    m_input_filename = filename;

    if (!m_reader->loadChannels(filename))
    {
        m_input_filename.clear();
        return;
    }

    m_file_loaded = true;
    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileTimesChanged();
    emit fileLoadedChanged();

    m_settings->loadFile(m_app_root + "/config/default.ini");
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

    m_settings->saveFile(m_app_root + "/config/last_used.ini");

    launchWorkerThread(params);
}

void MainViewModel::applySettings(const QString& frame_sync, bool neg_polarity,
                                 int scale_idx, const QString& range,
                                 int receiver_count, int channels_per_rcvr)
{
    m_settings_frame_sync = frame_sync;
    m_settings_neg_polarity = neg_polarity;
    m_settings_scale_idx = scale_idx;
    m_settings_range = range;

    int old_receiver_count = m_settings_receiver_count;
    int old_channel_count = m_settings_channels_per_rcvr;
    m_settings_receiver_count = receiver_count;
    m_settings_channels_per_rcvr = channels_per_rcvr;

    if (m_settings_receiver_count != old_receiver_count ||
        m_settings_channels_per_rcvr != old_channel_count)
    {
        m_receiver_states.resize(m_settings_receiver_count);
        for (int i = 0; i < m_settings_receiver_count; i++)
            m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
        emit receiverLayoutChanged();
    }

    emit settingsChanged();
}

void MainViewModel::loadSettings(const QString& filename)
{
    m_settings->loadFile(filename);
}

void MainViewModel::saveSettings(const QString& filename)
{
    m_settings->saveFile(filename);
}

void MainViewModel::clearState()
{
    QSettings config(m_app_root + "/config/default.ini", QSettings::IniFormat);
    m_settings_frame_sync = config.value("Defaults/FrameSync", PCMConstants::kDefaultFrameSync).toString();
    m_settings_neg_polarity = false;
    m_settings_scale_idx = config.value("Parameters/Scale", UIConstants::kDefaultScaleIndex).toInt();
    m_settings_range = config.value("Parameters/Range", UIConstants::kDefaultRange).toString();
    m_settings_receiver_count = config.value("Receivers/Count", UIConstants::kDefaultReceiverCount).toInt();
    m_settings_channels_per_rcvr =
        config.value("Receivers/ChannelsPerReceiver", UIConstants::kDefaultChannelsPerReceiver).toInt();

    m_input_filename.clear();
    m_last_output_file.clear();
    m_file_loaded = false;
    m_progress_percent = 0;
    m_processing = false;
    m_time_channel_index = 0;
    m_pcm_channel_index = 0;
    m_extract_all_time = true;
    m_sample_rate_index = 0;

    m_receiver_states.resize(m_settings_receiver_count);
    for (int i = 0; i < m_settings_receiver_count; i++)
        m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);

    m_reader->clearSettings();
    m_frame_setup->clearParameters();

    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileLoadedChanged();
    emit fileTimesChanged();
    emit progressPercentChanged();
    emit processingChanged();
    emit controlsEnabledChanged();
    emit settingsChanged();
    emit receiverLayoutChanged();
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
    if (params.sync_pattern_length > PCMConstants::kBitsInMinorFrame)
    {
        emit errorOccurred("Frame sync pattern (" +
                            QString::number(params.sync_pattern_length) +
                            " bits) exceeds frame length (" +
                            QString::number(PCMConstants::kBitsInMinorFrame) + " bits).");
        return false;
    }

    if (m_frame_setup->length() == 0)
    {
        emit errorOccurred("Frame setup not loaded.");
        return false;
    }

    int expected_param_count = m_settings_receiver_count * m_settings_channels_per_rcvr;
    if (m_frame_setup->length() != expected_param_count)
    {
        emit errorOccurred("Frame setup has " + QString::number(m_frame_setup->length()) +
                           " parameters but expected " + QString::number(expected_param_count) + ".");
        return false;
    }

    double range_dB = m_settings_range.toDouble();
    if (range_dB <= 0)
    {
        emit errorOccurred("Range must be a positive number.");
        return false;
    }
    int scale_index = m_settings_scale_idx;
    if (scale_index <= 1) // bipolar
    {
        params.scale_lower_bound = -range_dB / 2.0;
        params.scale_upper_bound = range_dB / 2.0;
    }
    else // unipolar
    {
        params.scale_lower_bound = 0;
        params.scale_upper_bound = range_dB;
    }

    params.negative_polarity = m_settings_neg_polarity;

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
        case 2: params.sample_rate = UIConstants::kSampleRate20Hz; break;
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
    QMap<QString, int> param_map;
    bool any_enabled = false;

    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        ParameterInfo* param = m_frame_setup->getParameter(i);
        param->slope = (scale_upper_bound - scale_lower_bound) / PCMConstants::kMaxRawSampleValue;
        param->scale = scale_lower_bound / (scale_upper_bound - scale_lower_bound) * PCMConstants::kMaxRawSampleValue;
        if (negative_polarity)
            param->scale *= -1;
        param->is_enabled = true;
        param_map.insert(param->name, i);
    }

    for (int receiver_index = 0; receiver_index < m_receiver_states.size(); receiver_index++)
    {
        for (int channel_index = 0;
             channel_index < m_receiver_states[receiver_index].size();
             channel_index++)
        {
            if (!m_receiver_states[receiver_index][channel_index])
            {
                QString name = parameterName(channel_index, receiver_index);
                auto it = param_map.constFind(name);
                if (it != param_map.constEnd())
                    m_frame_setup->getParameter(it.value())->is_enabled = false;
            }
        }
    }

    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        if (m_frame_setup->getParameter(i)->is_enabled)
        {
            any_enabled = true;
            break;
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
