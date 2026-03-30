/**
 * @file mainviewmodel.cpp
 * @brief Implementation of MainViewModel — state management and processing orchestration.
 */

#include "mainviewmodel.h"

#include <QCoreApplication>
#include <QDateTime>
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
      m_sample_rate_index(UIConstants::kDefaultSampleRateIndex),
      m_settings_frame_sync(PCMConstants::kDefaultFrameSync),
      m_settings_polarity_idx(UIConstants::kDefaultPolarityIndex),
      m_settings_slope_idx(UIConstants::kDefaultSlopeIndex),
      m_settings_scale(UIConstants::kDefaultScale),
      m_settings_receiver_count(UIConstants::kDefaultReceiverCount),
      m_settings_channels_per_rcvr(UIConstants::kDefaultChannelsPerReceiver),
      m_batch_mode(false),
      m_batch_current_index(0),
      m_batch_cancelled(false),
      m_batch_success_count(0),
      m_batch_skip_count(0),
      m_batch_error_count(0)
{
    QString app_dir = QCoreApplication::applicationDirPath();
    QString one_up = QDir::cleanPath(app_dir + "/..");
    QString two_up = QDir::cleanPath(app_dir + "/../..");
    if (QDir(app_dir + "/" + UIConstants::kSettingsDirName).exists())
    {
        m_app_root = app_dir;                       // portable: exe/ has settings/
    }
    else if (QDir(one_up + "/" + UIConstants::kSettingsDirName).exists())
    {
        m_app_root = one_up;                        // installed: bin/../ has settings/
    }
    else
    {
        m_app_root = two_up;                        // dev: build/release/../../ is project root
    }
    m_reader = new Chapter10Reader();
    m_frame_setup = new FrameSetup(this);
    m_settings = new SettingsManager(this);

    QSettings app_settings;
    m_last_ini_dir = app_settings.value(UIConstants::kSettingsKeyLastIniDir).toString();
    if (m_last_ini_dir.isEmpty())
    {
        m_last_ini_dir = m_app_root + "/" + UIConstants::kSettingsDirName;
    }

    QSettings config(m_app_root + "/" + UIConstants::kSettingsDirName + "/" + UIConstants::kDefaultIniFilename, QSettings::IniFormat);
    QString ini_sync = config.value("Frame/FrameSync").toString();
    if (!ini_sync.isEmpty())
    {
        m_settings_frame_sync = ini_sync;
    }
    m_settings_polarity_idx = config.value("Parameters/Polarity", UIConstants::kDefaultPolarityIndex).toInt();
    m_settings_slope_idx = config.value("Parameters/Slope", UIConstants::kDefaultSlopeIndex).toInt();
    QString ini_range = config.value("Parameters/Scale").toString();
    if (!ini_range.isEmpty())
    {
        m_settings_scale = ini_range;
    }
    m_settings_receiver_count = config.value("Receivers/Count", UIConstants::kDefaultReceiverCount).toInt();
    m_settings_channels_per_rcvr =
        config.value("Receivers/ChannelsPerReceiver", UIConstants::kDefaultChannelsPerReceiver).toInt();

    m_receiver_states.resize(m_settings_receiver_count);
    for (int i = 0; i < m_settings_receiver_count; i++)
    {
        m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
    }

    loadFrameSetupFrom(m_app_root + "/" + UIConstants::kSettingsDirName + "/" + UIConstants::kDefaultIniFilename);

    // Load recent files, pruning non-existent entries
    QStringList saved_recent = app_settings.value(UIConstants::kSettingsKeyRecentFiles).toStringList();
    for (const QString& path : saved_recent)
    {
        if (QFileInfo::exists(path))
        {
            m_recent_files.append(path);
        }
    }

    connect(m_reader, &Chapter10Reader::displayErrorMessage,
            this, &MainViewModel::errorOccurred);
    connect(m_settings, &SettingsManager::logMessage,
            this, &MainViewModel::logMessageReceived);
}

MainViewModel::~MainViewModel()
{
    if (m_worker_thread != nullptr && m_worker_thread->isRunning())
    {
        if (m_current_processor != nullptr)
        {
            m_current_processor->requestAbort();
        }
        m_worker_thread->quit();
        m_worker_thread->wait();
    }
    delete m_worker_thread;

    delete m_reader;
    delete m_frame_setup;
    delete m_settings;
}

////////////////////////////////////////////////////////////////////////////////
//                            PROPERTY GETTERS                                //
////////////////////////////////////////////////////////////////////////////////

QString MainViewModel::inputFilename() const
{
    if (m_batch_mode)
    {
        return batchStatusSummary();
    }
    return m_input_filename;
}

QStringList MainViewModel::timeChannelList() const
{
    if (m_batch_mode)
    {
        return {};
    }
    return m_reader->getTimeChannelComboBoxList();
}

QStringList MainViewModel::pcmChannelList() const
{
    if (m_batch_mode)
    {
        return {};
    }
    return m_reader->getPCMChannelComboBoxList();
}
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
    {
        return;
    }
    m_time_channel_index = index;
    if (!m_batch_mode)
    {
        m_reader->timeChannelChanged(index);
    }
    emit timeChannelIndexChanged();
}

void MainViewModel::setPcmChannelIndex(int index)
{
    if (m_pcm_channel_index == index)
    {
        return;
    }
    const int old_index = m_pcm_channel_index;
    m_pcm_channel_index = index;
    if (!m_batch_mode)
    {
        m_reader->pcmChannelChanged(index);
    }
    emit pcmChannelIndexChanged();

    // Re-run pre-scan only when the user explicitly changes the channel, not
    // when the channel is being initialised from 0 on file load (openFile()
    // already runs runPreScan() at the end of its own sequence).
    if (m_file_loaded && !m_batch_mode && old_index != 0)
    {
        runPreScan(m_reader->getCurrentPCMChannelID());
    }
}

void MainViewModel::setExtractAllTime(bool value)
{
    if (m_extract_all_time == value)
    {
        return;
    }
    m_extract_all_time = value;
    emit extractAllTimeChanged();
}

void MainViewModel::setSampleRateIndex(int value)
{
    if (m_sample_rate_index == value)
    {
        return;
    }
    m_sample_rate_index = value;
    emit sampleRateIndexChanged();
}

void MainViewModel::setFrameSync(const QString& value)
{
    if (m_settings_frame_sync == value)
    {
        return;
    }
    m_settings_frame_sync = value;
    emit settingsChanged();
}

void MainViewModel::setPolarityIndex(int value)
{
    if (m_settings_polarity_idx == value)
    {
        return;
    }
    m_settings_polarity_idx = value;
    emit settingsChanged();
}

void MainViewModel::setSlopeIndex(int value)
{
    if (m_settings_slope_idx == value)
    {
        return;
    }
    m_settings_slope_idx = value;
    emit settingsChanged();
}

void MainViewModel::setScale(const QString& value)
{
    if (m_settings_scale == value)
    {
        return;
    }
    m_settings_scale = value;
    emit settingsChanged();
}

void MainViewModel::setReceiverCount(int value)
{
    if (m_settings_receiver_count == value)
    {
        return;
    }
    m_settings_receiver_count = value;
    m_receiver_states.resize(value);
    for (int i = 0; i < value; i++)
    {
        if (m_receiver_states[i].size() != m_settings_channels_per_rcvr)
        {
            m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
        }
    }
    emit receiverLayoutChanged();
}

void MainViewModel::setChannelsPerReceiver(int value)
{
    if (m_settings_channels_per_rcvr == value)
    {
        return;
    }
    m_settings_channels_per_rcvr = value;
    for (qsizetype i = 0; i < m_receiver_states.size(); i++)
    {
        m_receiver_states[i].fill(true, value);
    }
    emit receiverLayoutChanged();
}

////////////////////////////////////////////////////////////////////////////////
//                          RECEIVER GRID STATE                               //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::receiverChecked(int receiver_index, int channel_index) const
{
    if (receiver_index < 0 || receiver_index >= m_receiver_states.size())
    {
        return false;
    }
    if (channel_index < 0 || channel_index >= m_receiver_states[receiver_index].size())
    {
        return false;
    }
    return m_receiver_states[receiver_index][channel_index];
}

void MainViewModel::setReceiverChecked(int receiver_index, int channel_index, bool checked)
{
    if (receiver_index < 0 || receiver_index >= m_receiver_states.size())
    {
        return;
    }
    if (channel_index < 0 || channel_index >= m_receiver_states[receiver_index].size())
    {
        return;
    }
    if (m_receiver_states[receiver_index][channel_index] == checked)
    {
        return;
    }
    m_receiver_states[receiver_index][channel_index] = checked;
    emit receiverCheckedChanged(receiver_index, channel_index, checked);
}

void MainViewModel::setAllReceiversChecked(bool checked)
{
    for (qsizetype r = 0; r < m_receiver_states.size(); r++)
    {
        for (qsizetype c = 0; c < m_receiver_states[r].size(); c++)
        {
            m_receiver_states[r][c] = checked;
        }
    }
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
        {
            m_receiver_states[i].fill(true, m_settings_channels_per_rcvr);
        }
        emit receiverLayoutChanged();
    }

    emit extractAllTimeChanged();
    emit sampleRateIndexChanged();
    emit settingsChanged();

    emit logMessageReceived("Settings applied:");
    emit logMessageReceived("  FrameSync=" + m_settings_frame_sync +
                            ", Polarity=" + QString(UIConstants::kPolarityLabels[m_settings_polarity_idx]) +
                            ", Slope=" + QString(UIConstants::kSlopeLabels[m_settings_slope_idx]) +
                            ", Scale=" + m_settings_scale + " dB/V");
    emit logMessageReceived("  Receivers=" + QString::number(m_settings_receiver_count) +
                            ", Channels=" + QString::number(m_settings_channels_per_rcvr));
}

void MainViewModel::loadFrameSetupFrom(const QString& filename)
{
    int words_in_frame = (m_settings_receiver_count * m_settings_channels_per_rcvr) + 1;
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
            {
                m_frame_setup->getParameter(it.value())->is_enabled = checked;
            }
        }
    }

    m_frame_setup->saveToSettings(settings);
}

////////////////////////////////////////////////////////////////////////////////
//                               HELPERS                                      //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::runPreScan(int pcm_channel_id)
{
    if (pcm_channel_id < 0)
    {
        emit logMessageReceived("Pre-scan: skipped — no PCM channel selected.");
        return false;
    }
    if (m_settings_frame_sync.isEmpty() || m_frame_setup->length() == 0)
    {
        emit logMessageReceived("Pre-scan: skipped — frame settings not loaded.");
        return false;
    }

    bool sync_ok = false;
    uint64_t scan_sync = m_settings_frame_sync.toULongLong(&sync_ok, UIConstants::kHexBase);
    if (!sync_ok)
    {
        return false;
    }

    int scan_sync_len = static_cast<int>(m_settings_frame_sync.length()) * 4;
    int data_words = m_frame_setup->length();
    int scan_words = data_words + 1;
    int scan_bits = (data_words * PCMConstants::kCommonWordLen) + scan_sync_len;

    ProcessingParams scan_params;
    scan_params.filename = m_input_filename;
    scan_params.pcm_channel_id = pcm_channel_id;
    scan_params.frame_sync = scan_sync;
    scan_params.sync_pattern_length = scan_sync_len;
    scan_params.words_in_minor_frame = scan_words;
    scan_params.bits_in_minor_frame = scan_bits;

    FrameProcessor scanner;
    connect(&scanner, &FrameProcessor::logMessage,
            this, &MainViewModel::logMessageReceived);

    return scanner.preScan(scan_params, m_is_randomized);
}

QMap<QString, int> MainViewModel::buildParameterMap() const
{
    QMap<QString, int> param_map;
    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        param_map.insert(m_frame_setup->getParameter(i)->name, i);
    }
    return param_map;
}

QString MainViewModel::channelPrefix(int index)
{
    if (index < UIConstants::kNumKnownPrefixes)
    {
        return UIConstants::kChannelPrefixes[index];
    }
    return "CH" + QString::number(index + 1);
}

QString MainViewModel::parameterName(int channel_index, int receiver_index)
{
    return channelPrefix(channel_index) + "_RCVR" + QString::number(receiver_index + 1);
}

QString MainViewModel::generateOutputFilename() const
{
    if (!m_input_filename.isEmpty())
    {
        return QString(UIConstants::kBatchOutputPrefix) +
               QFileInfo(m_input_filename).baseName() +
               UIConstants::kOutputExtension;
    }
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

QStringList MainViewModel::recentFiles() const { return m_recent_files; }

void MainViewModel::addRecentFile(const QString& filepath)
{
    m_recent_files.removeAll(filepath);
    m_recent_files.prepend(filepath);
    while (m_recent_files.size() > UIConstants::kMaxRecentFiles)
    {
        m_recent_files.removeLast();
    }

    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyRecentFiles, m_recent_files);
    emit recentFilesChanged();
}

void MainViewModel::clearRecentFiles()
{
    m_recent_files.clear();
    QSettings app_settings;
    app_settings.remove(UIConstants::kSettingsKeyRecentFiles);
    emit recentFilesChanged();
}

QString MainViewModel::fileMetadataSummary() const
{
    if (m_batch_mode)
    {
        int valid = batchValidCount();
        int skipped = batchSkippedCount();
        qint64 total_bytes = 0;
        for (const BatchFileInfo& info : m_batch_files)
        {
            total_bytes += info.fileSize;
        }
        QString size_str = (total_bytes >= UIConstants::kBytesPerMB)
            ? QString::number(static_cast<double>(total_bytes) / UIConstants::kBytesPerMB, 'f', 1) + " MB"
            : QString::number(static_cast<double>(total_bytes) / UIConstants::kBytesPerKB, 'f', 1) + " KB";
        return QString::number(m_batch_files.size()) + " files (" + size_str + ")  |  " +
            QString::number(valid) + " valid, " + QString::number(skipped) + " skipped";
    }

    if (!m_file_loaded)
    {
        return "No file loaded";
    }

    QFileInfo info(m_input_filename);
    qint64 bytes = info.size();
    QString size_str = (bytes >= UIConstants::kBytesPerMB)
        ? QString::number(static_cast<double>(bytes) / UIConstants::kBytesPerMB, 'f', 1) + " MB"
        : QString::number(static_cast<double>(bytes) / UIConstants::kBytesPerKB, 'f', 1) + " KB";

    int time_count = static_cast<int>(m_reader->getTimeChannelComboBoxList().size());
    int pcm_count = static_cast<int>(m_reader->getPCMChannelComboBoxList().size());

    QString time_range = QString("%1:%2:%3:%4 - %5:%6:%7:%8")
        .arg(m_reader->getStartDayOfYear(), 3, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStartHour(), 2, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStartMinute(), 2, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStartSecond(), 2, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStopDayOfYear(), 3, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStopHour(), 2, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStopMinute(), 2, UIConstants::kDecimalBase, QChar('0'))
        .arg(m_reader->getStopSecond(), 2, UIConstants::kDecimalBase, QChar('0'));

    return info.fileName() + "  |  " + size_str +
        "  |  Time: " + QString::number(time_count) +
        ", PCM: " + QString::number(pcm_count) +
        "  |  " + time_range;
}

////////////////////////////////////////////////////////////////////////////////
//                         BATCH PROCESSING                                   //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::batchMode() const { return m_batch_mode; }
int MainViewModel::batchFileCount() const { return static_cast<int>(m_batch_files.size()); }

int MainViewModel::batchValidCount() const
{
    int count = 0;
    for (const BatchFileInfo& info : m_batch_files)
    {
        if (!info.skip)
        {
            count++;
        }
    }
    return count;
}

int MainViewModel::batchSkippedCount() const
{
    int count = 0;
    for (const BatchFileInfo& info : m_batch_files)
    {
        if (info.skip)
        {
            count++;
        }
    }
    return count;
}

const QVector<BatchFileInfo>& MainViewModel::batchFiles() const { return m_batch_files; }

QString MainViewModel::generateBatchOutputFilename(const QString& input_filepath)
{
    return QString(UIConstants::kBatchOutputPrefix) +
           QFileInfo(input_filepath).baseName() +
           UIConstants::kOutputExtension;
}

QString MainViewModel::batchStatusSummary() const
{
    return m_batch_status_summary;
}

void MainViewModel::rebuildBatchStatusSummary()
{
    if (!m_batch_mode || m_batch_files.isEmpty())
    {
        m_batch_status_summary.clear();
        return;
    }
    m_batch_status_summary =
        QString::number(m_batch_files.size()) + " files loaded (" +
        QString::number(batchValidCount()) + " valid, " +
        QString::number(batchSkippedCount()) + " skipped)";
}

void MainViewModel::openFiles(const QStringList& filenames)
{
    if (filenames.size() == 1)
    {
        openFile(filenames.first());
        return;
    }

    clearState();

    m_batch_mode = true;
    emit batchModeChanged();

    emit logMessageReceived("--- Loading " + QString::number(filenames.size()) + " files ---");

    for (const QString& filepath : filenames)
    {
        BatchFileInfo info;
        info.filepath = filepath;
        info.filename = QFileInfo(filepath).fileName();
        info.fileSize = QFileInfo(filepath).size();

        Chapter10Reader temp_reader;
        if (temp_reader.loadChannels(filepath))
        {
            info.pcmChannelStrings = temp_reader.getPCMChannelComboBoxList();
            info.timeChannelStrings = temp_reader.getTimeChannelComboBoxList();

            for (const QString& s : info.pcmChannelStrings)
            {
                info.pcmChannelIds.append(s.split(" - ").first().toInt());
            }

            emit logMessageReceived("  Loaded: " + info.filename +
                " (PCM: " + QString::number(info.pcmChannelStrings.size()) +
                ", Time: " + QString::number(info.timeChannelStrings.size()) + ")");
        }
        else
        {
            info.skip = true;
            info.skipReason = "Failed to read file metadata";
            emit logMessageReceived("  WARNING: Could not load " + info.filename);
        }

        m_batch_files.append(info);
    }

    // Auto-resolve each file to its first PCM/time channel
    validateBatchFiles();

    m_file_loaded = true;

    for (const QString& filepath : filenames)
    {
        addRecentFile(filepath);
    }

    rebuildBatchStatusSummary();
    emit inputFilenameChanged();
    emit fileLoadedChanged();
    emit batchFilesChanged();
}

void MainViewModel::validateBatchFiles()
{
    if (!m_batch_mode || m_batch_files.isEmpty())
    {
        return;
    }

    for (BatchFileInfo& info : m_batch_files)
    {
        if (info.skipReason == "Failed to read file metadata")
        {
            continue;
        }

        info.skip = false;
        info.skipReason.clear();
        info.hasPcmChannel = !info.pcmChannelStrings.isEmpty();
        info.hasTimeChannel = !info.timeChannelStrings.isEmpty();

        // Auto-select first available channel in each file
        info.resolvedPcmIndex = info.hasPcmChannel ? 0 : -1;
        info.resolvedTimeIndex = info.hasTimeChannel ? 0 : -1;

        if (!info.hasPcmChannel)
        {
            info.skip = true;
            info.skipReason = "No PCM channels in file";
        }
        else if (!info.hasTimeChannel)
        {
            info.skip = true;
            info.skipReason = "No time channels in file";
        }
    }
}

void MainViewModel::setBatchFilePcmChannel(int fileIndex, int channelIndex)
{
    if (fileIndex < 0 || fileIndex >= m_batch_files.size())
    {
        return;
    }

    BatchFileInfo& info = m_batch_files[fileIndex];
    if (channelIndex < 0 || channelIndex >= info.pcmChannelStrings.size())
    {
        return;
    }

    info.resolvedPcmIndex = channelIndex;
    info.hasPcmChannel = true;

    // Re-evaluate skip status
    info.skip = false;
    info.skipReason.clear();
    if (info.resolvedTimeIndex < 0)
    {
        info.skip = true;
        info.skipReason = "No time channel selected";
    }

    emit batchFileUpdated(fileIndex);
}

void MainViewModel::setBatchFileTimeChannel(int fileIndex, int channelIndex)
{
    if (fileIndex < 0 || fileIndex >= m_batch_files.size())
    {
        return;
    }

    BatchFileInfo& info = m_batch_files[fileIndex];
    if (channelIndex < 0 || channelIndex >= info.timeChannelStrings.size())
    {
        return;
    }

    info.resolvedTimeIndex = channelIndex;
    info.hasTimeChannel = true;

    // Re-evaluate skip status
    info.skip = false;
    info.skipReason.clear();
    if (info.resolvedPcmIndex < 0)
    {
        info.skip = true;
        info.skipReason = "No PCM channel selected";
    }

    emit batchFileUpdated(fileIndex);
}

void MainViewModel::retryFailedFiles()
{
    if (m_processing || !m_batch_mode)
    {
        return;
    }

    // Reset only the files that failed — leave successful/skipped files untouched
    for (BatchFileInfo& info : m_batch_files)
    {
        if (info.processed && !info.processedOk && !info.skip)
        {
            info.processed = false;
            info.processedOk = false;
            info.preScanOk = false;  // Force re-scan with current channel selection
        }
    }

    m_batch_current_index = 0;
    m_batch_cancelled = false;
    m_batch_success_count = 0;
    m_batch_skip_count = 0;
    m_batch_error_count = 0;

    m_processing = true;
    m_progress_percent = 0;
    emit processingChanged();
    emit controlsEnabledChanged();
    emit progressPercentChanged();
    preScanBatchFiles();  // Re-scan reset files with current channel selection; emits batchFilesChanged()

    emit logMessageReceived("--- Batch Retry: Re-processing failed files ---");

    processNextBatchFile();
}

void MainViewModel::reorderBatchFile(int from, int to)
{
    const int count = m_batch_files.size();
    if (from < 0 || from >= count || to < 0 || to >= count || from == to)
    {
        return;
    }
    m_batch_files.move(from, to);
    rebuildBatchStatusSummary();
    emit batchFilesChanged();
}

void MainViewModel::preScanBatchFiles()
{
    if (!m_batch_mode)
    {
        return;
    }

    bool sync_ok = false;
    uint64_t scan_sync = m_settings_frame_sync.toULongLong(&sync_ok, UIConstants::kHexBase);
    if (!sync_ok)
    {
        return;
    }

    int scan_sync_len = static_cast<int>(m_settings_frame_sync.length()) * 4;
    int data_words = m_frame_setup->length();
    if (data_words == 0)
    {
        return;
    }
    int scan_words = data_words + 1;
    int scan_bits = (data_words * PCMConstants::kCommonWordLen) + scan_sync_len;

    for (BatchFileInfo& info : m_batch_files)
    {
        if (info.skip || info.processedOk)
        {
            continue;
        }

        int idx = info.resolvedPcmIndex;
        if (idx < 0 || idx >= info.pcmChannelIds.size())
        {
            info.preScanOk = false;
            continue;
        }
        int pcm_ch_id = info.pcmChannelIds[idx];

        ProcessingParams scan_params;
        scan_params.filename = info.filepath;
        scan_params.pcm_channel_id = pcm_ch_id;
        scan_params.frame_sync = scan_sync;
        scan_params.sync_pattern_length = scan_sync_len;
        scan_params.words_in_minor_frame = scan_words;
        scan_params.bits_in_minor_frame = scan_bits;

        FrameProcessor scanner;
        connect(&scanner, &FrameProcessor::logMessage,
                this, &MainViewModel::logMessageReceived);

        info.preScanOk = scanner.preScan(scan_params, info.isRandomized);
    }

    rebuildBatchStatusSummary();
    emit batchFilesChanged();
}

void MainViewModel::startBatchProcessing(const QString& output_dir, int sample_rate_index)
{
    if (m_worker_thread != nullptr && m_worker_thread->isRunning())
    {
        return;
    }

    m_batch_output_dir = output_dir;
    m_batch_current_index = 0;
    m_batch_cancelled = false;
    m_batch_success_count = 0;
    m_batch_skip_count = 0;
    m_batch_error_count = 0;

    // Batch mode always extracts the full time range of each file
    m_extract_all_time = true;
    m_batch_sample_rate_index = sample_rate_index;

    // Run pre-scan now (deferred from channel selection for batch mode)
    preScanBatchFiles();

    m_processing = true;
    m_progress_percent = 0;
    emit processingChanged();
    emit controlsEnabledChanged();
    emit progressPercentChanged();

    emit logMessageReceived("--- Batch Processing: " +
        QString::number(batchValidCount()) + " of " +
        QString::number(m_batch_files.size()) + " files ---");

    processNextBatchFile();
}

void MainViewModel::processNextBatchFile()
{
    while (m_batch_current_index < m_batch_files.size())
    {
        if (m_batch_cancelled)
        {
            emit logMessageReceived("Batch cancelled by user. Remaining files skipped.");
            break;
        }

        BatchFileInfo& info = m_batch_files[m_batch_current_index];

        if (info.skip)
        {
            emit logMessageReceived("  Skipping: " + info.filename + " (" + info.skipReason + ")");
            m_batch_skip_count++;
            m_batch_current_index++;
            continue;
        }

        // Already succeeded in a previous run — count it and move on (retry support)
        if (info.processed && info.processedOk)
        {
            m_batch_success_count++;
            m_batch_current_index++;
            continue;
        }

        if (!info.preScanOk)
        {
            info.processed = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Pre-scan failed (no frame sync) for " + info.filename + " — file skipped.");
            m_batch_current_index++;
            continue;
        }

        emit batchFileProcessing(m_batch_current_index, static_cast<int>(m_batch_files.size()));
        emit logMessageReceived("--- Processing file " +
            QString::number(m_batch_current_index + 1) + " of " +
            QString::number(m_batch_files.size()) + ": " + info.filename + " ---");

        m_reader->clearSettings();
        if (!m_reader->loadChannels(info.filepath))
        {
            info.processed = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Could not load " + info.filename);
            m_batch_current_index++;
            continue;
        }

        int pcm_idx = info.resolvedPcmIndex;
        int time_idx = info.resolvedTimeIndex;

        if (pcm_idx < 0 || time_idx < 0)
        {
            info.processed = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Channel resolution failed for " + info.filename);
            m_batch_current_index++;
            continue;
        }

        m_reader->pcmChannelChanged(pcm_idx + 1);
        m_reader->timeChannelChanged(time_idx + 1);

        ProcessingParams params;
        params.filename = info.filepath;
        params.time_channel_id = m_reader->getCurrentTimeChannelID();
        params.pcm_channel_id = m_reader->getCurrentPCMChannelID();

        bool frame_sync_ok = false;
        params.frame_sync = m_settings_frame_sync.toULongLong(&frame_sync_ok, UIConstants::kHexBase);
        params.sync_pattern_length = static_cast<int>(m_settings_frame_sync.length()) * 4;
        int data_words = m_frame_setup->length();
        params.words_in_minor_frame = data_words + 1;
        params.bits_in_minor_frame = (data_words * PCMConstants::kCommonWordLen) + params.sync_pattern_length;

        double scale_dB_per_V = m_settings_scale.toDouble();
        int scale_index = m_settings_slope_idx;
        double voltage_lower = UIConstants::kSlopeVoltageLower[scale_index];
        double voltage_upper = UIConstants::kSlopeVoltageUpper[scale_index];
        params.calibration.scale_lower_bound = voltage_lower * scale_dB_per_V;
        params.calibration.scale_upper_bound = voltage_upper * scale_dB_per_V;
        params.calibration.negative_polarity = (m_settings_polarity_idx == 1);

        // Batch mode always uses each file's full time range
        params.start_seconds = m_reader->dhmsToUInt64(
            m_reader->getStartDayOfYear(), m_reader->getStartHour(),
            m_reader->getStartMinute(), m_reader->getStartSecond());
        params.stop_seconds = m_reader->dhmsToUInt64(
            m_reader->getStopDayOfYear(), m_reader->getStopHour(),
            m_reader->getStopMinute(), m_reader->getStopSecond());

        switch (m_batch_sample_rate_index)
        {
            case 0: params.sample_rate = UIConstants::kSampleRate1Hz; break;
            case 1: params.sample_rate = UIConstants::kSampleRate10Hz; break;
            case 2: params.sample_rate = UIConstants::kSampleRate100Hz; break;
            default: params.sample_rate = UIConstants::kSampleRate1Hz; break;
        }

        params.outfile = m_batch_output_dir + "/" +
            UIConstants::kBatchOutputPrefix + QFileInfo(info.filepath).baseName() +
            UIConstants::kOutputExtension;
        info.outputFile = params.outfile;
        params.is_randomized = info.isRandomized;

        if (!prepareFrameSetupParameters(params.calibration))
        {
            info.processed = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: No receivers selected for " + info.filename);
            m_batch_current_index++;
            continue;
        }

        launchWorkerThread(params);
        return;
    }

    m_processing = false;
    m_progress_percent = UIConstants::kProgressBarMax;
    emit progressPercentChanged();
    emit processingChanged();
    emit controlsEnabledChanged();

    emit logMessageReceived("--- Batch Complete ---");
    emit logMessageReceived("  Success: " + QString::number(m_batch_success_count) +
        ", Skipped: " + QString::number(m_batch_skip_count) +
        ", Errors: " + QString::number(m_batch_error_count) +
        " / Total: " + QString::number(m_batch_files.size()));

    rebuildBatchStatusSummary();
    emit batchFilesChanged();
    emit processingFinished(m_batch_error_count == 0 && m_batch_success_count > 0,
                            m_batch_output_dir);
}

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
    QString size_str = (file_bytes >= UIConstants::kBytesPerMB)
        ? QString::number(static_cast<double>(file_bytes) / UIConstants::kBytesPerMB, 'f', 1) + " MB"
        : QString::number(static_cast<double>(file_bytes) / UIConstants::kBytesPerKB, 'f', 1) + " KB";
    int duration_sec = ((m_reader->getStopDayOfYear() - m_reader->getStartDayOfYear()) * UIConstants::kSecondsPerDay)
        + ((m_reader->getStopHour() - m_reader->getStartHour()) * UIConstants::kSecondsPerHour)
        + ((m_reader->getStopMinute() - m_reader->getStartMinute()) * UIConstants::kSecondsPerMinute)
        + (m_reader->getStopSecond() - m_reader->getStartSecond());
    emit logMessageReceived("  File size: " + size_str +
        ", Recording duration: " + QString::number(duration_sec) + "s");

    // Log channels found
    QStringList time_list = m_reader->getTimeChannelComboBoxList();
    QStringList pcm_list = m_reader->getPCMChannelComboBoxList();
    emit logMessageReceived("  Time channels: " + (time_list.isEmpty() ? "none" : QString::number(time_list.size())));
    for (const QString& ch : time_list)
    {
        emit logMessageReceived("    " + ch);
    }
    emit logMessageReceived("  PCM channels: " + (pcm_list.isEmpty() ? "none" : QString::number(pcm_list.size())));
    for (const QString& ch : pcm_list)
    {
        emit logMessageReceived("    " + ch);
    }

    // Log file time range
    emit logMessageReceived("  Time range: " +
        QString("%1:%2:%3:%4")
            .arg(m_reader->getStartDayOfYear(), 3, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStartHour(), 2, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStartMinute(), 2, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStartSecond(), 2, UIConstants::kDecimalBase, QChar('0')) +
        " - " +
        QString("%1:%2:%3:%4")
            .arg(m_reader->getStopDayOfYear(), 3, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStopHour(), 2, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStopMinute(), 2, UIConstants::kDecimalBase, QChar('0'))
            .arg(m_reader->getStopSecond(), 2, UIConstants::kDecimalBase, QChar('0')));

    // Log current frame settings
    emit logMessageReceived("  FrameSync=" + m_settings_frame_sync +
        ", Polarity=" + QString(UIConstants::kPolarityLabels[m_settings_polarity_idx]) +
        ", Slope=" + QString(UIConstants::kSlopeLabels[m_settings_slope_idx]) +
        ", Scale=" + m_settings_scale + " dB/V");
    emit logMessageReceived("  Receivers=" + QString::number(m_settings_receiver_count) +
        ", Channels=" + QString::number(m_settings_channels_per_rcvr) +
        ", Frame setup=" + QString::number(m_frame_setup->length()) + " parameters");

    m_file_loaded = true;
    addRecentFile(filename);
    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileTimesChanged();
    emit fileLoadedChanged();

    runPreScan(m_reader->getCurrentPCMChannelID());
}

void MainViewModel::startProcessing(const QString& output_file,
                                     const QString& start_time,
                                     const QString& stop_time,
                                     int sample_rate_index)
{
    ProcessingParams params;
    if (!validateProcessingInputs(params, start_time, stop_time, sample_rate_index))
    {
        return;
    }

    if (!prepareFrameSetupParameters(params.calibration))
    {
        emit errorOccurred("No receivers selected.");
        return;
    }

    params.outfile = output_file;
    m_last_output_file = output_file;

    // Guard against re-entry
    if (m_worker_thread != nullptr && m_worker_thread->isRunning())
    {
        return;
    }

    // Log pre-process summary
    emit logMessageReceived("--- Processing Summary ---");
    emit logMessageReceived("  Input: " + QFileInfo(params.filename).fileName());
    emit logMessageReceived("  Time Ch: " + QString::number(params.time_channel_id) +
        ", PCM Ch: " + QString::number(params.pcm_channel_id));
    if (m_extract_all_time)
    {
        emit logMessageReceived("  Time range: all");
    }
    else
    {
        emit logMessageReceived("  Time range: " +
            QString::number(params.start_seconds) + "s - " +
            QString::number(params.stop_seconds) + "s");
    }
    emit logMessageReceived("  Sample rate: " + QString::number(params.sample_rate) + " Hz");

    int enabled = 0;
    for (const auto& row : m_receiver_states)
    {
        for (bool checked : row)
        {
            if (checked)
            {
                enabled++;
            }
        }
    }
    emit logMessageReceived("  Receivers: " + QString::number(enabled) + " / " +
        QString::number(m_settings_receiver_count * m_settings_channels_per_rcvr) + " enabled");
    emit logMessageReceived("  Output: " + params.outfile);

    if (!runPreScan(params.pcm_channel_id))
    {
        emit errorOccurred("Pre-scan failed: no frame sync found. Processing aborted.");
        return;
    }

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

    bool was_batch = m_batch_mode;
    m_batch_files.clear();
    m_batch_mode = false;
    m_batch_current_index = 0;
    m_batch_cancelled = false;
    m_batch_output_dir.clear();
    m_batch_success_count = 0;
    m_batch_skip_count = 0;
    m_batch_error_count = 0;

    m_reader->clearSettings();

    emit inputFilenameChanged();
    emit channelListsChanged();
    emit fileLoadedChanged();
    emit fileTimesChanged();
    emit progressPercentChanged();
    emit processingChanged();
    emit controlsEnabledChanged();
    if (was_batch)
    {
        emit batchModeChanged();
    }
    rebuildBatchStatusSummary();
    emit batchFilesChanged();
}

void MainViewModel::cancelProcessing()
{
    if (m_current_processor != nullptr)
    {
        m_current_processor->requestAbort();
    }
    if (m_batch_mode)
    {
        m_batch_cancelled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
//                        VALIDATION & PROCESSING                             //
////////////////////////////////////////////////////////////////////////////////

bool MainViewModel::validateTimeFields(const QString& ddd, const QString& hh,
                                        const QString& mm, const QString& ss,
                                        TimeFields& out)
{
    bool ddd_ok = false;
    bool hh_ok = false;
    bool mm_ok = false;
    bool ss_ok = false;
    out.ddd = ddd.toInt(&ddd_ok);
    out.hh = hh.toInt(&hh_ok);
    out.mm = mm.toInt(&mm_ok);
    out.ss = ss.toInt(&ss_ok);

    return ddd_ok && hh_ok && mm_ok && ss_ok &&
           out.ddd >= UIConstants::kMinDayOfYear && out.ddd <= UIConstants::kMaxDayOfYear &&
           out.hh >= 0 && out.hh <= UIConstants::kMaxHour &&
           out.mm >= 0 && out.mm <= UIConstants::kMaxMinute &&
           out.ss >= 0 && out.ss <= UIConstants::kMaxSecond;
}

QString MainViewModel::validateTimeRange(const QString& start_text,
                                          const QString& stop_text)
{
    QStringList start_parts = start_text.split(":");
    QStringList stop_parts = stop_text.split(":");

    if (start_parts.size() != 4 || stop_parts.size() != 4)
    {
        return "Start and stop times must be in DDD:HH:MM:SS format.";
    }

    TimeFields s;
    if (!validateTimeFields(start_parts[0], start_parts[1],
                             start_parts[2], start_parts[3], s))
    {
        return "Start time is out of valid range. Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.";
    }

    TimeFields e;
    if (!validateTimeFields(stop_parts[0], stop_parts[1],
                             stop_parts[2], stop_parts[3], e))
    {
        return "Stop time is out of valid range. Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.";
    }

    long long start_total = (s.ddd * (long long)UIConstants::kSecondsPerDay)
        + (s.hh * (long long)UIConstants::kSecondsPerHour)
        + (s.mm * (long long)UIConstants::kSecondsPerMinute) + s.ss;
    long long stop_total = (e.ddd * (long long)UIConstants::kSecondsPerDay)
        + (e.hh * (long long)UIConstants::kSecondsPerHour)
        + (e.mm * (long long)UIConstants::kSecondsPerMinute) + e.ss;

    if (stop_total <= start_total)
    {
        return "Stop time must be after start time.";
    }

    return {};
}

bool MainViewModel::validateProcessingInputs(ProcessingParams& params,
                                              const QString& start_time,
                                              const QString& stop_time,
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

    bool frame_sync_ok = false;
    params.frame_sync = m_settings_frame_sync.toULongLong(&frame_sync_ok, UIConstants::kHexBase);
    if (!frame_sync_ok)
    {
        emit errorOccurred("Invalid frame sync.");
        return false;
    }

    params.sync_pattern_length = static_cast<int>(m_settings_frame_sync.length()) * 4;
    if (params.sync_pattern_length <= 0)
    {
        emit errorOccurred("Frame sync pattern is empty.");
        return false;
    }

    int data_words = m_frame_setup->length();
    params.words_in_minor_frame = data_words + 1;
    params.bits_in_minor_frame = (data_words * PCMConstants::kCommonWordLen) + params.sync_pattern_length;

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
    params.calibration.scale_lower_bound = voltage_lower * scale_dB_per_V;
    params.calibration.scale_upper_bound = voltage_upper * scale_dB_per_V;

    params.calibration.negative_polarity = (m_settings_polarity_idx == 1);

    QStringList start_parts = start_time.split(":");
    if (start_parts.size() != 4)
    {
        emit errorOccurred("Invalid start time format.");
        return false;
    }
    TimeFields s;
    if (!validateTimeFields(start_parts[0], start_parts[1],
                             start_parts[2], start_parts[3], s))
    {
        emit errorOccurred("Invalid start time.");
        return false;
    }
    params.start_seconds = m_reader->dhmsToUInt64(s.ddd, s.hh, s.mm, s.ss);

    QStringList stop_parts = stop_time.split(":");
    if (stop_parts.size() != 4)
    {
        emit errorOccurred("Invalid stop time format.");
        return false;
    }
    TimeFields e;
    if (!validateTimeFields(stop_parts[0], stop_parts[1],
                             stop_parts[2], stop_parts[3], e))
    {
        emit errorOccurred("Invalid stop time.");
        return false;
    }
    params.stop_seconds = m_reader->dhmsToUInt64(e.ddd, e.hh, e.mm, e.ss);

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

    params.is_randomized = m_is_randomized;
    return true;
}

bool MainViewModel::prepareFrameSetupParameters(const CalibrationParams& cal)
{
    bool any_enabled = false;

    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        ParameterInfo* param = m_frame_setup->getParameter(i);
        param->slope = (cal.scale_upper_bound - cal.scale_lower_bound) / PCMConstants::kMaxRawSampleValue;
        if (cal.negative_polarity)
        {
            param->slope *= -1;
            param->scale = -cal.scale_upper_bound / (cal.scale_upper_bound - cal.scale_lower_bound) * PCMConstants::kMaxRawSampleValue;
        }
        else
        {
            param->scale = cal.scale_lower_bound / (cal.scale_upper_bound - cal.scale_lower_bound) * PCMConstants::kMaxRawSampleValue;
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
    if (m_worker_thread != nullptr)
    {
        m_worker_thread->quit();
        m_worker_thread->wait();
        delete m_worker_thread;
        m_worker_thread = nullptr;
        m_current_processor = nullptr;
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

    connect(m_worker_thread, &QThread::started, processor, [processor, params, this]() {
        processor->process(params, m_frame_setup);
    });

    m_worker_thread->start();
}

void MainViewModel::onProgressUpdated(int percent)
{
    if (m_batch_mode)
    {
        int total = static_cast<int>(m_batch_files.size());
        if (total > 0)
        {
            m_progress_percent = ((m_batch_current_index * UIConstants::kProgressBarMax) + percent) / total;
        }
    }
    else
    {
        m_progress_percent = percent;
    }
    emit progressPercentChanged();
}

void MainViewModel::onProcessingFinished(bool success)
{
    m_current_processor = nullptr;

    m_worker_thread->quit();
    m_worker_thread->wait();
    delete m_worker_thread;
    m_worker_thread = nullptr;

    if (m_batch_mode)
    {
        BatchFileInfo& info = m_batch_files[m_batch_current_index];
        info.processed = true;
        info.processedOk = success;

        if (success)
        {
            m_batch_success_count++;
            emit logMessageReceived("  Completed: " + info.filename + " -> " + info.outputFile);
        }
        else
        {
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Processing failed for " + info.filename);
        }

        m_batch_current_index++;

        int file_progress = (m_batch_current_index * UIConstants::kProgressBarMax) / static_cast<int>(m_batch_files.size());
        m_progress_percent = file_progress;
        emit progressPercentChanged();

        processNextBatchFile();
    }
    else
    {
        if (success)
        {
            m_progress_percent = UIConstants::kProgressBarMax;
        }

        m_processing = false;
        emit progressPercentChanged();
        emit processingChanged();
        emit controlsEnabledChanged();
        emit processingFinished(success, m_last_output_file);
    }
}

void MainViewModel::onLogMessage(const QString& message)
{
    emit logMessageReceived(message);
}

#ifdef QT_TESTLIB_LIB
void MainViewModel::addBatchFileForTesting(const BatchFileInfo& info)
{
    m_batch_files.append(info);
    m_batch_mode = true;
}
#endif
