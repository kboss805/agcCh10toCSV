/**
 * @file processingcoordinator.cpp
 * @brief Implementation of ProcessingCoordinator — worker thread lifecycle and batch sequencing.
 */

#include "processingcoordinator.h"

#include <QFileInfo>
#include <QMap>

#include "chapter10reader.h"
#include "constants.h"
#include "frameprocessor.h"
#include "framesetup.h"

ProcessingCoordinator::ProcessingCoordinator(QVector<BatchFileInfo>* batch_files,
                                             FrameSetup*             frame_setup,
                                             Chapter10Reader*        reader,
                                             QObject*                parent)
    : QObject(parent),
      m_batch_files(batch_files),
      m_frame_setup(frame_setup),
      m_reader(reader)
{
    Q_ASSERT(batch_files != nullptr);
    Q_ASSERT(frame_setup != nullptr);
    Q_ASSERT(reader != nullptr);
}

ProcessingCoordinator::~ProcessingCoordinator()
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
}

////////////////////////////////////////////////////////////////////////////////
//                            STATE QUERIES                                   //
////////////////////////////////////////////////////////////////////////////////

bool  ProcessingCoordinator::processing()      const { return m_processing; }
int   ProcessingCoordinator::progressPercent() const { return m_progress_percent; }
bool  ProcessingCoordinator::isRandomized()    const { return m_is_randomized; }

////////////////////////////////////////////////////////////////////////////////
//                            PUBLIC API                                      //
////////////////////////////////////////////////////////////////////////////////

bool ProcessingCoordinator::startSingleProcessing(const ProcessingParams&        params,
                                                   const QVector<QVector<bool>>& receiver_states)
{
    m_receiver_states = receiver_states;
    if (!prepareFrameSetupParameters(params.calibration))
    {
        emit errorOccurred("No receivers selected.");
        return false;
    }
    m_last_output_file = params.outfile;
    m_processing       = true;
    m_progress_percent = 0;
    emit processingStateChanged(true);
    emit progressChanged(0);
    launchWorkerThread(params);
    return true;
}

void ProcessingCoordinator::startBatchProcessing(const QString&              output_dir,
                                                  int                         sample_rate_index,
                                                  const QString&              frame_sync_str,
                                                  int                         polarity_idx,
                                                  int                         slope_idx,
                                                  const QString&              scale_str,
                                                  const QVector<QVector<bool>>& receiver_states)
{
    m_batch_output_dir         = output_dir;
    m_batch_current_index      = 0;
    m_batch_cancelled          = false;
    m_batch_success_count      = 0;
    m_batch_skip_count         = 0;
    m_batch_error_count        = 0;
    m_batch_sample_rate_index  = sample_rate_index;
    m_frame_sync_str           = frame_sync_str;
    m_polarity_idx             = polarity_idx;
    m_slope_idx                = slope_idx;
    m_scale_str                = scale_str;
    m_receiver_states          = receiver_states;

    preScanBatchFiles();

    m_processing       = true;
    m_progress_percent = 0;
    emit processingStateChanged(true);
    emit progressChanged(0);

    emit logMessageReceived("--- Batch Processing: " +
        QString::number(m_batch_files->size()) + " files ---");

    processNextBatchFile();
}

void ProcessingCoordinator::retryFailedFiles(const QString&              frame_sync_str,
                                              int                         polarity_idx,
                                              int                         slope_idx,
                                              const QString&              scale_str,
                                              const QVector<QVector<bool>>& receiver_states)
{
    m_frame_sync_str  = frame_sync_str;
    m_polarity_idx    = polarity_idx;
    m_slope_idx       = slope_idx;
    m_scale_str       = scale_str;
    m_receiver_states = receiver_states;

    for (BatchFileInfo& info : *m_batch_files)
    {
        if (info.processed && !info.processedOk && !info.skip)
        {
            info.processed   = false;
            info.processedOk = false;
            info.preScanOk   = false;
        }
    }

    m_batch_current_index = 0;
    m_batch_cancelled     = false;
    m_batch_success_count = 0;
    m_batch_skip_count    = 0;
    m_batch_error_count   = 0;

    preScanBatchFiles();

    m_processing       = true;
    m_progress_percent = 0;
    emit processingStateChanged(true);
    emit progressChanged(0);

    emit logMessageReceived("--- Batch Retry: Re-processing failed files ---");
    processNextBatchFile();
}

void ProcessingCoordinator::cancelProcessing()
{
    if (m_current_processor != nullptr)
    {
        m_current_processor->requestAbort();
    }
    m_batch_cancelled = true;
}

bool ProcessingCoordinator::runPreScan(int            pcm_channel_id,
                                        const QString& filename,
                                        const QString& frame_sync_str)
{
    if (pcm_channel_id < 0)
    {
        emit logMessageReceived("Pre-scan: skipped — no PCM channel selected.");
        return false;
    }
    if (frame_sync_str.isEmpty() || m_frame_setup->length() == 0)
    {
        emit logMessageReceived("Pre-scan: skipped — frame settings not loaded.");
        return false;
    }

    bool sync_ok = false;
    uint64_t scan_sync = frame_sync_str.toULongLong(&sync_ok, UIConstants::kHexBase);
    if (!sync_ok)
    {
        return false;
    }

    int scan_sync_len = static_cast<int>(frame_sync_str.length()) * 4;
    int data_words    = m_frame_setup->length();
    int scan_words    = data_words + 1;
    int scan_bits     = (data_words * PCMConstants::kCommonWordLen) + scan_sync_len;

    ProcessingParams scan_params;
    scan_params.filename             = filename;
    scan_params.pcm_channel_id       = pcm_channel_id;
    scan_params.frame_sync           = scan_sync;
    scan_params.sync_pattern_length  = scan_sync_len;
    scan_params.words_in_minor_frame = scan_words;
    scan_params.bits_in_minor_frame  = scan_bits;

    FrameProcessor scanner;
    connect(&scanner, &FrameProcessor::logMessage,
            this, &ProcessingCoordinator::logMessageReceived);

    return scanner.preScan(scan_params, m_is_randomized);
}

void ProcessingCoordinator::reset()
{
    m_batch_current_index = 0;
    m_batch_cancelled     = false;
    m_batch_output_dir.clear();
    m_batch_success_count = 0;
    m_batch_skip_count    = 0;
    m_batch_error_count   = 0;
    m_progress_percent    = 0;
    m_processing          = false;
    m_last_output_file.clear();
}

////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE HELPERS                                 //
////////////////////////////////////////////////////////////////////////////////

QMap<QString, int> ProcessingCoordinator::buildParameterMap() const
{
    QMap<QString, int> param_map;
    for (int i = 0; i < m_frame_setup->length(); i++)
    {
        param_map.insert(m_frame_setup->getParameter(i)->name, i);
    }
    return param_map;
}

bool ProcessingCoordinator::prepareFrameSetupParameters(const CalibrationParams& cal)
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

    QMap<QString, int> param_map = buildParameterMap();

    for (int receiver_index = 0; receiver_index < m_receiver_states.size(); receiver_index++)
    {
        for (int channel_index = 0;
             channel_index < m_receiver_states[receiver_index].size();
             channel_index++)
        {
            if (m_receiver_states[receiver_index][channel_index])
            {
                // Build the parameter name using the same static helpers as ViewModel
                int ci = channel_index;
                int ri = receiver_index;
                QString prefix = (ci < UIConstants::kNumKnownPrefixes)
                    ? UIConstants::kChannelPrefixes[ci]
                    : "CH" + QString::number(ci + 1);
                QString name = prefix + "_RCVR" + QString::number(ri + 1);

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

void ProcessingCoordinator::launchWorkerThread(const ProcessingParams& params)
{
    if (m_worker_thread != nullptr)
    {
        m_worker_thread->quit();
        m_worker_thread->wait();
        delete m_worker_thread;
        m_worker_thread     = nullptr;
        m_current_processor = nullptr;
    }

    m_worker_thread = new QThread;

    auto* processor  = new FrameProcessor;
    m_current_processor = processor;
    processor->moveToThread(m_worker_thread);

    connect(processor, &FrameProcessor::progressUpdated,
            this, &ProcessingCoordinator::onProgressUpdated);
    connect(processor, &FrameProcessor::processingFinished,
            this, &ProcessingCoordinator::onProcessingFinished);
    connect(processor, &FrameProcessor::logMessage,
            this, &ProcessingCoordinator::onLogMessage);
    connect(processor, &FrameProcessor::errorOccurred,
            this, &ProcessingCoordinator::errorOccurred);

    connect(m_worker_thread, &QThread::finished,
            processor, &QObject::deleteLater);

    connect(m_worker_thread, &QThread::started, processor, [processor, params, this]() {
        processor->process(params, m_frame_setup);
    });

    m_worker_thread->start();
}

void ProcessingCoordinator::preScanBatchFiles()
{
    bool sync_ok = false;
    uint64_t scan_sync = m_frame_sync_str.toULongLong(&sync_ok, UIConstants::kHexBase);
    if (!sync_ok)
    {
        return;
    }

    int scan_sync_len = static_cast<int>(m_frame_sync_str.length()) * 4;
    int data_words    = m_frame_setup->length();
    if (data_words == 0)
    {
        return;
    }
    int scan_words = data_words + 1;
    int scan_bits  = (data_words * PCMConstants::kCommonWordLen) + scan_sync_len;

    for (BatchFileInfo& info : *m_batch_files)
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
        scan_params.filename             = info.filepath;
        scan_params.pcm_channel_id       = pcm_ch_id;
        scan_params.frame_sync           = scan_sync;
        scan_params.sync_pattern_length  = scan_sync_len;
        scan_params.words_in_minor_frame = scan_words;
        scan_params.bits_in_minor_frame  = scan_bits;

        FrameProcessor scanner;
        connect(&scanner, &FrameProcessor::logMessage,
                this, &ProcessingCoordinator::logMessageReceived);

        info.preScanOk = scanner.preScan(scan_params, info.isRandomized);
    }

    emit batchFilesUpdated();
}

void ProcessingCoordinator::processNextBatchFile()
{
    while (m_batch_current_index < m_batch_files->size())
    {
        if (m_batch_cancelled)
        {
            emit logMessageReceived("Batch cancelled by user. Remaining files skipped.");
            break;
        }

        BatchFileInfo& info = (*m_batch_files)[m_batch_current_index];

        if (info.skip)
        {
            emit logMessageReceived("  Skipping: " + info.filename + " (" + info.skipReason + ")");
            m_batch_skip_count++;
            m_batch_current_index++;
            continue;
        }

        if (info.processed && info.processedOk)
        {
            m_batch_success_count++;
            m_batch_current_index++;
            continue;
        }

        if (!info.preScanOk)
        {
            info.processed   = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Pre-scan failed (no frame sync) for " + info.filename + " — file skipped.");
            m_batch_current_index++;
            continue;
        }

        emit batchFileProcessing(m_batch_current_index, static_cast<int>(m_batch_files->size()));
        emit logMessageReceived("--- Processing file " +
            QString::number(m_batch_current_index + 1) + " of " +
            QString::number(m_batch_files->size()) + ": " + info.filename + " ---");

        m_reader->clearSettings();
        if (!m_reader->loadChannels(info.filepath))
        {
            info.processed   = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Could not load " + info.filename);
            m_batch_current_index++;
            continue;
        }

        int pcm_idx  = info.resolvedPcmIndex;
        int time_idx = info.resolvedTimeIndex;

        if (pcm_idx < 0 || time_idx < 0)
        {
            info.processed   = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: Channel not selected for " + info.filename);
            m_batch_current_index++;
            continue;
        }

        m_reader->pcmChannelChanged(pcm_idx + 1);
        m_reader->timeChannelChanged(time_idx + 1);

        ProcessingParams params;
        params.filename        = info.filepath;
        params.time_channel_id = m_reader->getCurrentTimeChannelID();
        params.pcm_channel_id  = m_reader->getCurrentPCMChannelID();

        bool frame_sync_ok = false;
        params.frame_sync            = m_frame_sync_str.toULongLong(&frame_sync_ok, UIConstants::kHexBase);
        params.sync_pattern_length   = static_cast<int>(m_frame_sync_str.length()) * 4;
        int data_words               = m_frame_setup->length();
        params.words_in_minor_frame  = data_words + 1;
        params.bits_in_minor_frame   = (data_words * PCMConstants::kCommonWordLen) + params.sync_pattern_length;

        double scale_dB_per_V                   = m_scale_str.toDouble();
        double voltage_lower                     = UIConstants::kSlopeVoltageLower[m_slope_idx];
        double voltage_upper                     = UIConstants::kSlopeVoltageUpper[m_slope_idx];
        params.calibration.scale_lower_bound    = voltage_lower * scale_dB_per_V;
        params.calibration.scale_upper_bound    = voltage_upper * scale_dB_per_V;
        params.calibration.negative_polarity    = (m_polarity_idx == 1);

        params.start_seconds = m_reader->dhmsToUInt64(
            m_reader->getStartDayOfYear(), m_reader->getStartHour(),
            m_reader->getStartMinute(),    m_reader->getStartSecond());
        params.stop_seconds  = m_reader->dhmsToUInt64(
            m_reader->getStopDayOfYear(),  m_reader->getStopHour(),
            m_reader->getStopMinute(),     m_reader->getStopSecond());

        switch (m_batch_sample_rate_index)
        {
            case 0:  params.sample_rate = UIConstants::kSampleRate1Hz;   break;
            case 1:  params.sample_rate = UIConstants::kSampleRate10Hz;  break;
            case 2:  params.sample_rate = UIConstants::kSampleRate100Hz; break;
            default: params.sample_rate = UIConstants::kSampleRate1Hz;   break;
        }

        params.outfile = m_batch_output_dir + "/" +
            UIConstants::kBatchOutputPrefix + QFileInfo(info.filepath).baseName() +
            UIConstants::kOutputExtension;
        info.outputFile       = params.outfile;
        params.is_randomized  = info.isRandomized;

        if (!prepareFrameSetupParameters(params.calibration))
        {
            info.processed   = true;
            info.processedOk = false;
            m_batch_error_count++;
            emit logMessageReceived("  ERROR: No receivers selected for " + info.filename);
            m_batch_current_index++;
            continue;
        }

        launchWorkerThread(params);
        return;
    }

    // All files processed (or batch cancelled)
    m_processing       = false;
    m_progress_percent = UIConstants::kProgressBarMax;
    emit progressChanged(m_progress_percent);
    emit processingStateChanged(false);

    emit logMessageReceived("--- Batch Complete ---");
    emit logMessageReceived("  Success: " + QString::number(m_batch_success_count) +
        ", Skipped: " + QString::number(m_batch_skip_count) +
        ", Errors: "  + QString::number(m_batch_error_count) +
        " / Total: "  + QString::number(m_batch_files->size()));

    emit batchFilesUpdated();
    emit processingFinished(m_batch_error_count == 0 && m_batch_success_count > 0,
                            m_batch_output_dir);
}

////////////////////////////////////////////////////////////////////////////////
//                            SLOTS                                           //
////////////////////////////////////////////////////////////////////////////////

void ProcessingCoordinator::onProgressUpdated(int percent)
{
    int total = static_cast<int>(m_batch_files->size());
    if (total > 0 && m_batch_current_index < total)
    {
        m_progress_percent = ((m_batch_current_index * UIConstants::kProgressBarMax) + percent) / total;
    }
    else
    {
        m_progress_percent = percent;
    }
    emit progressChanged(m_progress_percent);
}

void ProcessingCoordinator::onProcessingFinished(bool success)
{
    m_current_processor = nullptr;

    m_worker_thread->quit();
    m_worker_thread->wait();
    delete m_worker_thread;
    m_worker_thread = nullptr;

    if (!m_batch_files->isEmpty() && m_batch_current_index < m_batch_files->size())
    {
        // Batch mode: update file result and advance state machine
        BatchFileInfo& info = (*m_batch_files)[m_batch_current_index];
        info.processed   = true;
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

        int file_progress = (m_batch_current_index * UIConstants::kProgressBarMax)
                            / static_cast<int>(m_batch_files->size());
        m_progress_percent = file_progress;
        emit progressChanged(m_progress_percent);

        processNextBatchFile();
    }
    else
    {
        // Single-file mode
        if (success)
        {
            m_progress_percent = UIConstants::kProgressBarMax;
        }
        m_processing = false;
        emit progressChanged(m_progress_percent);
        emit processingStateChanged(false);
        emit processingFinished(success, m_last_output_file);
    }
}

void ProcessingCoordinator::onLogMessage(const QString& message)
{
    emit logMessageReceived(message);
}
