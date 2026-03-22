/**
 * @file frameprocessor.cpp
 * @brief Implementation of FrameProcessor — PCM frame extraction and CSV output.
 */

#include "frameprocessor.h"

#include <ctime>
#include <utility>

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QVector>

#include "constants.h"
#include "framesetup.h"
#include "i106_decode_pcmf1.h"
#include "i106_decode_time.h"

using namespace Irig106;

namespace {
    // Time threshold for gap detection in seconds
    constexpr double kTimeGapThreshold = 2.0;
    // Conversion factor from 100ns units to seconds
    constexpr double k100NsToSeconds = 1.0e-7;
    // Percentage reporting intervals
    constexpr int kPercent100 = 100;
    constexpr int kPercent10 = 10;
}

////////////////////////////////////////////////////////////////////////////////
//                          IRIG106 HELPER METHODS                            //
////////////////////////////////////////////////////////////////////////////////

void FrameProcessor::freeChanInfoTable(QVector<SuChanInfo*>& channel_info)
{
    for(auto* info : channel_info)
    {
        if(info != nullptr)
        {
            if(info->psuAttributes != nullptr)
            {
                if (strcasecmp(info->psuRDataSrc->szChannelDataType,"PCMIN") == 0)
                {
                    FreeOutputBuffers_PcmF1(static_cast<SuPcmF1_Attributes*>(info->psuAttributes));
                }
                free(info->psuAttributes); // NOLINT(cppcoreguidelines-no-malloc, cppcoreguidelines-owning-memory)
                info->psuAttributes = nullptr;
            }

            delete info;
        }
    }
    // Clear and reset to null pointers
    channel_info.clear();
    channel_info.resize(PCMConstants::kMaxChannelCount, nullptr);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
EnI106Status FrameProcessor::assembleAttributesFromTMATS(SuTmatsInfo* tmats_info,
                                                         QVector<SuChanInfo*>& channel_info)
{
    SuRRecord* psuRRecord = nullptr;
    SuRDataSource* psuRDataSrc = nullptr;
    int iTrackNumber = 0;

    if((tmats_info->psuFirstGRecord == nullptr) || (tmats_info->psuFirstRRecord == nullptr))
    {
        // For simplicity in this legacy port, we just logging error
        return(I106_INVALID_DATA);
    }

    // Ensure vector is sized correctly
    if (channel_info.size() < PCMConstants::kMaxChannelCount)
    {
        channel_info.resize(PCMConstants::kMaxChannelCount, nullptr);
    }

    psuRRecord = tmats_info->psuFirstRRecord;
    while (psuRRecord != nullptr)
    {
        psuRDataSrc = psuRRecord->psuFirstDataSource;
        while (psuRDataSrc != nullptr)
        {
            if(psuRDataSrc->szTrackNumber == nullptr)
            {
                // Uninitialized track number, skip
                psuRDataSrc = psuRDataSrc->psuNext;
                continue;
            }

            iTrackNumber = atoi(psuRDataSrc->szTrackNumber);

            if(iTrackNumber >= PCMConstants::kMaxChannelCount)
            {
                return(I106_BUFFER_TOO_SMALL);
            }

            if (channel_info[iTrackNumber] == nullptr)
            {
                channel_info[iTrackNumber] = new SuChanInfo();
                memset(channel_info[iTrackNumber], 0, sizeof(SuChanInfo));

                channel_info[iTrackNumber]->uChID = static_cast<uint16_t>(iTrackNumber);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                channel_info[iTrackNumber]->bEnabled = (psuRDataSrc->szEnabled[0] == 'T') ? 1 : 0;
                channel_info[iTrackNumber]->psuRDataSrc = psuRDataSrc;

                if (strcasecmp(psuRDataSrc->szChannelDataType,"PCMIN") == 0)
                {
                    // Allocation using calloc to match legacy C API expectations
                    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
                    channel_info[iTrackNumber]->psuAttributes = calloc(1, sizeof(SuPcmF1_Attributes));
                    if(channel_info[iTrackNumber]->psuAttributes == nullptr)
                    {
                        freeChanInfoTable(channel_info);
                        return(I106_BUFFER_TOO_SMALL);
                    }
                    (void)Set_Attributes_PcmF1(psuRDataSrc, static_cast<SuPcmF1_Attributes*>(channel_info[iTrackNumber]->psuAttributes));
                }
            }

            psuRDataSrc = psuRDataSrc->psuNext;
        }

        psuRRecord = psuRRecord->psuNext;
    }

    return(I106_OK);
}

////////////////////////////////////////////////////////////////////////////////
//                          PCM BIT-LEVEL HELPERS                             //
////////////////////////////////////////////////////////////////////////////////

// Static method
void FrameProcessor::derandomizeBitstream(uint8_t* data, uint64_t total_bits, uint16_t& lfsr)
{
    // Constants for RNRZ-L derandomization
    const uint16_t kLfsrMask = 0x7FFF;
    const int kTap1 = 13;
    const int kTap2 = 14;

    for (uint64_t i = 0; i < total_bits; i++)
    {
        uint32_t byte_idx = static_cast<uint32_t>(i / 8);
        uint8_t bit_idx = static_cast<uint8_t>(7 - (i % 8));

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        int in_bit  = (data[byte_idx] >> bit_idx) & 1;
        int lfsr_out_bit = ((lfsr >> kTap1) & 1) ^ ((lfsr >> kTap2) & 1);
        int descrambled_bit = in_bit ^ lfsr_out_bit;
        lfsr = ((lfsr << 1) | in_bit) & kLfsrMask;

        if (descrambled_bit != 0)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            data[byte_idx] |= static_cast<uint8_t>(1 << bit_idx);
        }
        else
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            data[byte_idx] &= static_cast<uint8_t>(~(1 << bit_idx));
        }
    }
}

// Static method
bool FrameProcessor::hasSyncPattern(const uint8_t* data, uint64_t total_bits,
                                    uint64_t sync_pat, uint64_t sync_mask,
                                    uint32_t sync_pat_len)
{
    uint64_t test_word = 0;
    uint64_t bits_loaded = 0;
    const uint8_t kHighBitMask = 0x80;

    for (uint64_t i = 0; i < total_bits; i++)
    {
        uint32_t byte_idx = static_cast<uint32_t>(i / 8);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        uint8_t bit_val = ((data[byte_idx] & (kHighBitMask >> (i % 8))) != 0) ? 1 : 0;
        test_word = (test_word << 1) | bit_val;
        bits_loaded++;
        if (bits_loaded >= sync_pat_len &&
            (test_word & sync_mask) == sync_pat)
        {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//                       CONSTRUCTOR / DESTRUCTOR                             //
////////////////////////////////////////////////////////////////////////////////

FrameProcessor::FrameProcessor(QObject* parent)
    : QObject(parent),
      m_total_file_size(0),
      m_abort_requested(false)
{
    putenv("TZ=GMT0");
    tzset();

    m_channel_info.resize(PCMConstants::kMaxChannelCount, nullptr);
    m_buffer.resize(PCMConstants::kDefaultBufferSize);
}

FrameProcessor::~FrameProcessor()
{
    freeChanInfoTable(m_channel_info);
}

void FrameProcessor::requestAbort()
{
    m_abort_requested.store(true, std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////
//                            PRE-SCAN                                        //
////////////////////////////////////////////////////////////////////////////////

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool FrameProcessor::preScan(const QString& filename,
                              int pcm_channel_id,
                              uint64_t frame_sync,
                              int sync_pattern_len,
                              int words_in_minor_frame,
                              int bits_in_minor_frame,
                              bool& is_randomized,
                              int max_packets)
{
    emit logMessage("Pre-scan: checking frame sync and encoding...");

    if (pcm_channel_id < 0 || pcm_channel_id >= PCMConstants::kMaxChannelCount)
    {
        emit logMessage("Pre-scan: skipped — no PCM channel available.");
        return false;
    }

    if (!openFile(filename))
    {
        emit logMessage("Pre-scan: skipped — could not open file.");
        return false;
    }

    // Read TMATS (first packet)
    m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);
    if (m_status != I106_OK || m_header.ubyDataType != I106CH10_DTYPE_TMATS)
    {
        emit logMessage("Pre-scan: skipped — could not read TMATS.");
        closeFile();
        return false;
    }

    if (!ensureBufferCapacity(static_cast<qsizetype>(m_header.ulPacketLen)))
    {
        emit logMessage("Pre-scan: skipped — memory allocation failed.");
        closeFile();
        return false;
    }

    m_status = enI106Ch10ReadData(m_file_handle, static_cast<unsigned long>(m_buffer.size()), m_buffer.data());
    if (m_status != I106_OK)
    {
        emit logMessage("Pre-scan: skipped — could not read TMATS data.");
        closeFile();
        return false;
    }

    memset(&m_tmats_info, 0, sizeof(m_tmats_info));
    m_status = enI106_Decode_Tmats(&m_header, m_buffer.data(), &m_tmats_info);
    if (m_status != I106_OK)
    {
        emit logMessage("Pre-scan: skipped — could not decode TMATS.");
        closeFile();
        return false;
    }

    m_status = assembleAttributesFromTMATS(&m_tmats_info, m_channel_info);
    if (m_status != I106_OK)
    {
        emit logMessage("Pre-scan: skipped — could not assemble channel attributes.");
        closeFile();
        return false;
    }

    // Set up PCM attributes for the target channel
    if (m_channel_info[pcm_channel_id] == nullptr)
    {
        emit logMessage("Pre-scan: skipped — PCM channel " +
                        QString::number(pcm_channel_id) + " not found in TMATS.");
        closeFile();
        return false;
    }

    // Use static_cast for safer type conversion
    auto* pcm_attrs = static_cast<SuPcmF1_Attributes*>(m_channel_info[pcm_channel_id]->psuAttributes);
    if (pcm_attrs == nullptr)
    {
        emit logMessage("Pre-scan: skipped — no PCM attributes for channel.");
        closeFile();
        return false;
    }

    Set_Attributes_Ext_PcmF1(pcm_attrs->psuRDataSrc, pcm_attrs,
                              -1, -1,
                              PCMConstants::kCommonWordLen,
                              -1, -1, -1,
                              PCMConstants::kNumMinorFrames,
                              words_in_minor_frame,
                              bits_in_minor_frame,
                              -1,
                              sync_pattern_len,
                              static_cast<int64_t>(frame_sync),
                              -1, -1, -1);

    uint64_t sync_pat  = pcm_attrs->ullMinorFrameSyncPat;
    uint64_t sync_mask = pcm_attrs->ullMinorFrameSyncMask;
    uint32_t sync_len  = pcm_attrs->ulMinorFrameSyncPatLen;

    // -----------------------------------------------------------------------
    // Pass 1: NRZ-L scan — check raw (non-randomized) data for frame sync.
    //         Cache the byte-swapped packet bytes for possible pass 2.
    // -----------------------------------------------------------------------
    int pcm_packets_scanned = 0;
    int nrzl_sync_count  = 0;
    int rnrzl_sync_count = 0;
    QVector<QByteArray> cached_packets;

    while (pcm_packets_scanned < max_packets)
    {
        m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);
        if (m_status == I106_EOF || m_status != I106_OK)
        {
            break;
        }

        if (m_header.ubyDataType != I106CH10_DTYPE_PCM_FMT_1 ||
            m_header.uChID != pcm_channel_id)
        {
            continue;
        }

        if (!ensureBufferCapacity(static_cast<qsizetype>(m_header.ulPacketLen)))
        {
            break;
        }

        m_status = enI106Ch10ReadData(m_file_handle, static_cast<unsigned long>(m_buffer.size()), m_buffer.data());
        if (m_status != I106_OK)
        {
            break;
        }

        uint32_t data_offset = sizeof(SuPcmF1_ChanSpec);
        if (m_header.ulDataLen <= data_offset)
        {
            continue;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* raw_data = reinterpret_cast<uint8_t*>(m_buffer.data() + data_offset);
        uint32_t raw_len  = m_header.ulDataLen - data_offset;

        if (pcm_attrs->bDontSwapRawData == 0)
        {
            SwapBytes_PcmF1(raw_data, static_cast<long>(raw_len));
        }

        uint64_t packet_bits = static_cast<uint64_t>(raw_len) * 8;

        // Test NRZ-L: sync pattern in raw (non-randomized) data.
        if (hasSyncPattern(raw_data, packet_bits, sync_pat, sync_mask, sync_len))
        {
            nrzl_sync_count++;
        }

        // Cache byte-swapped packet for RNRZ-L pass if needed.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        cached_packets.append(
            QByteArray(reinterpret_cast<const char*>(raw_data), static_cast<qsizetype>(raw_len)));

        pcm_packets_scanned++;
    }

    closeFile();

    // -----------------------------------------------------------------------
    // Pass 2: RNRZ-L scan — only run if NRZ-L found no sync at all.
    //         Derandomize each cached packet and search for frame sync.
    // -----------------------------------------------------------------------
    if (nrzl_sync_count == 0 && !cached_packets.isEmpty())
    {
        for (const QByteArray& pkt : std::as_const(cached_packets))
        {
            QByteArray derand = pkt;
            uint16_t test_lfsr = 0;
            const uint64_t pkt_bits = static_cast<uint64_t>(pkt.size()) * 8;
            derandomizeBitstream(reinterpret_cast<uint8_t*>(derand.data()), pkt_bits, test_lfsr);

            if (hasSyncPattern(reinterpret_cast<const uint8_t*>(derand.constData()),
                               pkt_bits, sync_pat, sync_mask, sync_len))
            {
                rnrzl_sync_count++;
            }
        }
    }

    // Report findings
    if (pcm_packets_scanned == 0)
    {
        emit logMessage("Pre-scan: no PCM packets found for channel " +
                        QString::number(pcm_channel_id) + ".");
        return false;
    }

    emit logMessage("Pre-scan: scanned " + QString::number(pcm_packets_scanned) +
                    " PCM packet(s) on channel " + QString::number(pcm_channel_id) + ".");
    emit logMessage("  NRZ-L  sync found in " + QString::number(nrzl_sync_count) +
                    " of " + QString::number(pcm_packets_scanned) + " packets.");
    emit logMessage("  RNRZ-L sync found in " + QString::number(rnrzl_sync_count) +
                    " of " + QString::number(pcm_packets_scanned) + " packets.");

    // NRZ-L is checked first; RNRZ-L pass only ran if NRZ-L found nothing,
    // so there is no ambiguous "both found" case.
    is_randomized = false;
    bool sync_found = false;
    if (nrzl_sync_count > 0)
    {
        emit logMessage("Pre-scan result: data appears to be NRZ-L (not randomized).");
        sync_found = true;
    }
    else if (rnrzl_sync_count > 0)
    {
        emit logMessage("Pre-scan result: data appears to be RNRZ-L (randomized).");
        is_randomized = true;
        sync_found = true;
    }
    else
    {
        emit logMessage("WARNING: Pre-scan — frame sync pattern NOT found. "
                        "Verify frame sync and PCM channel.");
    }

    return sync_found;
}

////////////////////////////////////////////////////////////////////////////////
//                            FILE I/O                                        //
////////////////////////////////////////////////////////////////////////////////

bool FrameProcessor::ensureBufferCapacity(qsizetype required)
{
    if (m_buffer.size() >= required)
        return true;
    try {
        m_buffer.resize(required);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool FrameProcessor::openFile(const QString& filename)
{
    m_status = enI106Ch10Open(&m_file_handle, filename.toUtf8().constData(), I106_READ);

    if (m_status != I106_OK && m_status != I106_OPEN_WARNING)
    {
        emit errorOccurred("Error opening data file.");
        return false;
    }

    m_status = enI106_SyncTime(m_file_handle, bFALSE, 0);

    if (m_status != I106_OK)
    {
        emit errorOccurred("Error establishing time sync.");
        return false;
    }

    return true;
}

void FrameProcessor::closeFile() const
{
    if (m_file_handle >= 0)
    {
        enI106Ch10Close(m_file_handle);
    }
    // Vector clears automatically
}

////////////////////////////////////////////////////////////////////////////////
//                          PROCESSING                                        //
////////////////////////////////////////////////////////////////////////////////

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool FrameProcessor::process(const QString& filename,
                                FrameSetup* frame_setup,
                                const QString& outfile,
                                int time_channel_id,
                                int pcm_channel_id,
                                uint64_t frame_sync,
                                int sync_pattern_len,
                                int words_in_minor_frame,
                                int bits_in_minor_frame,
                                uint64_t start_seconds,
                                uint64_t stop_seconds,
                                int sample_rate,
                                bool is_randomized)
{
    QElapsedTimer elapsed_timer;
    elapsed_timer.start();

    m_total_file_size = QFileInfo(filename).size();
    int last_reported_percent = -1;

    // Validate channel IDs before using them as array indices
    if (time_channel_id < 0 || time_channel_id >= PCMConstants::kMaxChannelCount)
    {
        emit errorOccurred("Time channel ID is out of range.");
        emit processingFinished(false);
        return false;
    }
    if (pcm_channel_id < 0 || pcm_channel_id >= PCMConstants::kMaxChannelCount)
    {
        emit errorOccurred("PCM channel ID is out of range.");
        emit processingFinished(false);
        return false;
    }

    // Clear channel info for this run
    freeChanInfoTable(m_channel_info);

    // Open input file and sync time
    emit logMessage("Opening Chapter 10 file...");
    if (!openFile(filename))
    {
        emit errorOccurred("Failed to load Chapter 10 file.");
        emit processingFinished(false);
        return false;
    }

    // Open output file
    emit logMessage("Creating output CSV file...");
    QFile output(outfile);
    if (!output.open(QIODevice::WriteOnly))
    {
        emit errorOccurred("Failed to open output file: " + outfile);
        closeFile();
        emit processingFinished(false);
        return false;
    }

    // Cleanup helper for error paths
    auto fail = [&](const QString& msg) -> bool {
        emit errorOccurred(msg);
        if (output.isOpen()) { output.close(); }
        closeFile();
        emit processingFinished(false);
        return false;
    };

    // Pre-cache enabled parameters to avoid repeated iteration in hot loops
    QVector<ParameterInfo*> enabled_params;
    enabled_params.reserve(frame_setup->length());
    for (int i = 0; i < frame_setup->length(); i++)
    {
        ParameterInfo* param = frame_setup->getParameter(i);
        if (param->is_enabled)
        {
            enabled_params.push_back(param);
        }
    }

    // Write CSV header
    QString header_line = QStringLiteral("Day,Time");
    for (const auto* param : enabled_params)
    {
        header_line += ',' + param->name;
    }
    header_line += '\n';
    output.write(header_line.toUtf8());

    // Read and process the first packet (must be TMATS)
    emit logMessage("Reading TMATS metadata...");
    m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);

    if (m_status != I106_OK)
    {
        return fail("Failed to read first header.");
    }

    if (m_header.ubyDataType == I106CH10_DTYPE_TMATS)
    {
        if (!ensureBufferCapacity(static_cast<qsizetype>(m_header.ulPacketLen)))
        {
            return fail("Memory allocation failed.");
        }

        m_status = enI106Ch10ReadData(m_file_handle, static_cast<unsigned long>(m_buffer.size()), m_buffer.data());
        if (m_status != I106_OK)
        {
            return fail("Failed to read data from first header.");
        }

        memset(&m_tmats_info, 0, sizeof(m_tmats_info));
        m_status = enI106_Decode_Tmats(&m_header, m_buffer.data(), &m_tmats_info);
        if (m_status != I106_OK)
        {
            return fail("Failed to process TMATS info from first header.");
        }

        m_status = assembleAttributesFromTMATS(&m_tmats_info, m_channel_info);
        if (m_status != I106_OK)
        {
            return fail("Failed to assemble attributes from TMATS header.");
        }
    }
    else
    {
        return fail("Failed to find TMATS message.");
    }

    // Set up PCM attributes for the selected channel
    emit logMessage("Setting up PCM attributes...");
    if (m_channel_info[pcm_channel_id] == nullptr)
    {
        return fail("Channel info not set up for selected PCM channel.");
    }

    auto* pcm_attrs = static_cast<SuPcmF1_Attributes*>(m_channel_info[pcm_channel_id]->psuAttributes);
    if (pcm_attrs == nullptr)
    {
        return fail("Unable to load PCM attributes.");
    }

    Set_Attributes_Ext_PcmF1(pcm_attrs->psuRDataSrc, pcm_attrs,
                              -1, // lRecordNum
                              -1, // lBitsPerSec
                              PCMConstants::kCommonWordLen,
                              -1, // lWordTransferOrder
                              -1, // lParityType
                              -1, // lParityTransferOrder
                              PCMConstants::kNumMinorFrames,
                              words_in_minor_frame,
                              bits_in_minor_frame,
                              -1, // lMinorFrameSyncType
                              sync_pattern_len,
                              static_cast<int64_t>(frame_sync), // llMinorFrameSyncPat
                              -1, // lMinSyncs
                              -1, // llMinorFrameSyncMask
                              -1); // lNoByteSwap (use TMATS default)

    // -----------------------------------------------------------------------
    // Set up frame extraction state machine
    // -----------------------------------------------------------------------
    uint64_t sync_pat = pcm_attrs->ullMinorFrameSyncPat;
    uint64_t sync_mask = pcm_attrs->ullMinorFrameSyncMask;
    uint32_t sync_pat_len = pcm_attrs->ulMinorFrameSyncPatLen;
    uint32_t bits_in_frame = pcm_attrs->ulBitsInMinorFrame;
    uint32_t words_in_frame = pcm_attrs->ulWordsInMinorFrame;
    uint32_t word_len = pcm_attrs->ulCommonWordLen;
    uint64_t word_mask = pcm_attrs->ullCommonWordMask;
    double delta_100ns = pcm_attrs->dDelta100NanoSeconds;

    uint64_t test_word = 0;
    uint64_t bits_loaded = 0;
    uint32_t minor_frame_bit_count = 0;
    uint32_t minor_frame_word_count = 0;
    uint32_t data_word_bit_count = 0;
    int32_t save_data = 0;     // 0=waiting, 1=collecting, 2=frame complete
    uint64_t sync_count = UINT64_MAX; // -1 equivalent: no sync found yet
    uint64_t total_syncs_found = 0;
    uint64_t total_frames_extracted = 0;
    uint64_t total_bytes_processed = 0;
    uint64_t rows_written = 0;

    QVector<uint64_t> frame_words(words_in_frame, 0);

    // CSV output state
    double sample_period = 1.0 / static_cast<double>(sample_rate);
    double current_time_sample = static_cast<double>(start_seconds);
    double next_time_sample = current_time_sample + sample_period;
    int n_samples = 0;

    for (auto* param : enabled_params)
    {
        param->sample_sum = 0;
    }

    // Timestamp tracking: keep current and previous packet time references
    uint64_t global_bit_offset = 0;
    PacketTimeRef current_time_ref = {0, 0, 0};
    PacketTimeRef prev_time_ref = {0, 0, 0};
    bool has_time_ref = false;

    // Time gap detection
    double prev_time_seconds = -1.0;
    int time_gaps_detected = 0;

    // Derandomization state (determined by preScan encoding result)
    bool needs_derand = is_randomized;
    uint16_t lfsr_state = 0;

    // -----------------------------------------------------------------------
    // Single pass: read packets and process PCM data immediately
    // -----------------------------------------------------------------------
    emit logMessage("Processing PCM data...");
    emit logMessage(QString("Time window: start=%1s stop=%2s")
                    .arg(start_seconds).arg(stop_seconds));
    int packet_count = 0;

    while (true)
    {
        m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);
        if (m_status == I106_EOF)
        {
            break;
        }
        if (m_status != I106_OK)
        {
            emit errorOccurred("File read error during data collection.");
            break;
        }

        if (m_abort_requested.load(std::memory_order_relaxed))
        {
            emit logMessage("Processing cancelled by user.");
            emit processingFinished(false);
            return false;
        }

        // Report progress every N packets to reduce I/O overhead
        packet_count++;
        if (m_total_file_size > 0 && (packet_count % PCMConstants::kProgressReportInterval) == 0)
        {
            int64_t current_pos = 0;
            enI106Ch10GetPos(m_file_handle, &current_pos);
            int percent = static_cast<int>(current_pos * kPercent100 / m_total_file_size);
            if (percent != last_reported_percent)
            {
                if (percent / kPercent10 != last_reported_percent / kPercent10 && percent > 0)
                {
                    emit logMessage(QString::number(percent) + "% complete...");
                }
                last_reported_percent = percent;
                emit progressUpdated(percent);
            }
        }

        // Process IRIG time packets to maintain time sync
        if (m_header.ubyDataType == I106CH10_DTYPE_IRIG_TIME && m_header.uChID == time_channel_id)
        {
            if (!ensureBufferCapacity(static_cast<qsizetype>(m_header.ulPacketLen)))
            {
                emit errorOccurred("Memory allocation failed.");
                break;
            }

            m_status = enI106Ch10ReadData(m_file_handle, static_cast<unsigned long>(m_buffer.size()), m_buffer.data());
            if (m_status != I106_OK)
            {
                emit errorOccurred("File read error; aborting parsing.");
                break;
            }

            enI106_Decode_TimeF1(&m_header, m_buffer.data(), &m_irig_time);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            enI106_SetRelTime(m_file_handle, &m_irig_time, m_header.aubyRefTime);

            double pkt_time = static_cast<double>(m_irig_time.ulSecs) +
                              (k100NsToSeconds * static_cast<double>(m_irig_time.ulFrac));
            if (prev_time_seconds >= 0)
            {
                double gap = pkt_time - prev_time_seconds;
                if (gap > kTimeGapThreshold)
                {
                    time_gaps_detected++;
                    auto gap_epoch = static_cast<time_t>(pkt_time);
                    struct tm* gt = gmtime(&gap_epoch);
                    if (gt != nullptr)
                    {
                        constexpr int kBase10 = 10;
                        emit logMessage(QString("WARNING: Time gap of %1s at DOY %2 %3:%4:%5")
                            .arg(gap, 0, 'f', 1)
                            .arg(gt->tm_yday + 1, 3, kBase10, QChar('0'))
                            .arg(gt->tm_hour, 2, kBase10, QChar('0'))
                            .arg(gt->tm_min, 2, kBase10, QChar('0'))
                            .arg(gt->tm_sec, 2, kBase10, QChar('0')));
                    }
                }
            }
            prev_time_seconds = pkt_time;
        }

        // Process PCM data from the selected channel
        if (m_header.ubyDataType == I106CH10_DTYPE_PCM_FMT_1 && m_header.uChID == pcm_channel_id)
        {
            if (!ensureBufferCapacity(static_cast<qsizetype>(m_header.ulPacketLen)))
            {
                emit errorOccurred("Memory allocation failed.");
                break;
            }

            m_status = enI106Ch10ReadData(m_file_handle, static_cast<unsigned long>(m_buffer.size()), m_buffer.data());
            if (m_status != I106_OK)
            {
                emit errorOccurred("File read error; aborting parsing.");
                break;
            }

            // Skip the 4-byte SuPcmF1_ChanSpec header to get raw PCM data
            uint32_t data_offset = sizeof(SuPcmF1_ChanSpec);
            if (m_header.ulDataLen <= data_offset)
            {
                continue;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto* raw_data = reinterpret_cast<uint8_t*>(m_buffer.data() + data_offset);
            uint32_t raw_len = m_header.ulDataLen - data_offset;
            uint64_t packet_bits = static_cast<uint64_t>(raw_len) * 8;

            // Byte-swap raw data if needed (library default: swap)
            if (pcm_attrs->bDontSwapRawData == 0)
            {
                SwapBytes_PcmF1(raw_data, static_cast<long>(raw_len));
            }

            if (needs_derand)
            {
                derandomizeBitstream(raw_data, packet_bits, lfsr_state);
            }

            // Update time references (keep current + previous for boundary frames)
            int64_t pkt_base_time = 0;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            vTimeArray2LLInt(m_header.aubyRefTime, &pkt_base_time);

            if (has_time_ref)
            {
                prev_time_ref = current_time_ref;
            }

            current_time_ref.base_time = pkt_base_time;
            current_time_ref.start_bit = global_bit_offset;
            current_time_ref.num_bits = packet_bits;
            has_time_ref = true;

            constexpr uint64_t kAbortCheckMask = 0xFFFF;
            // Process all bits in this packet through the frame extraction state machine
            for (uint64_t bit_pos = 0; bit_pos < packet_bits; bit_pos++)
            {
                if ((bit_pos & kAbortCheckMask) == 0 && m_abort_requested.load(std::memory_order_relaxed))
                {
                    emit logMessage("Processing cancelled by user.");
                    emit processingFinished(false);
                    return false;
                }

                uint32_t mbyte_idx = static_cast<uint32_t>(bit_pos / 8);
                constexpr uint8_t kHighBit = 0x80;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                uint8_t bit_val = ((raw_data[mbyte_idx] & (kHighBit >> (bit_pos % 8))) != 0) ? 1 : 0;

                test_word = (test_word << 1) | bit_val;
                bits_loaded++;
                minor_frame_bit_count++;

                // Check for sync word
                if (bits_loaded >= sync_pat_len &&
                    (test_word & sync_mask) == sync_pat)
                {
                    total_syncs_found++;

                    if (minor_frame_bit_count == bits_in_frame)
                    {
                        sync_count++;

                        if (sync_count >= pcm_attrs->ulMinSyncs && save_data > 1)
                        {
                            // Compute per-frame time using bit-level interpolation
                            uint64_t global_bit_pos = global_bit_offset + bit_pos;
                            uint64_t frame_start_bit = global_bit_pos + 1 - bits_in_frame;

                            const PacketTimeRef& ref =
                                (frame_start_bit >= current_time_ref.start_bit)
                                    ? current_time_ref : prev_time_ref;

                            int64_t frame_rel_time = ref.base_time +
                                static_cast<int64_t>(
                                    static_cast<double>(frame_start_bit - ref.start_bit) * delta_100ns);

                            enI106_RelInt2IrigTime(m_file_handle, frame_rel_time, &m_irig_time);
                            double current_time = (k100NsToSeconds * static_cast<double>(m_irig_time.ulFrac))
                                                  + static_cast<double>(m_irig_time.ulSecs);
                            bool write_samples = false;

                            if (current_time >= static_cast<double>(start_seconds) && current_time <= static_cast<double>(stop_seconds))
                            {
                                if (next_time_sample < current_time)
                                {
                                    if (n_samples > 0)
                                    {
                                        writeTimeSample(output, current_time_sample, n_samples, enabled_params);
                                        rows_written++;
                                    }

                                    n_samples = 0;
                                    write_samples = true;
                                }

                                if (write_samples)
                                {
                                    while (next_time_sample < current_time)
                                    {
                                        current_time_sample += sample_period;
                                        next_time_sample += sample_period;
                                    }
                                }

                                for (auto* param : enabled_params)
                                {
                                    if (param->word >= 0 &&
                                        param->word < static_cast<int>(words_in_frame))
                                    {
                                        int64_t raw_value = static_cast<int64_t>(frame_words[param->word] & word_mask);
                                        double scaled_value = (static_cast<double>(raw_value) + param->scale) * param->slope;
                                        param->sample_sum += scaled_value;
                                    }
                                }

                                n_samples++;
                                total_frames_extracted++;
                            }
                        }
                    }

                    minor_frame_bit_count = 0;
                    minor_frame_word_count = 1;
                    data_word_bit_count = 0;
                    save_data = 1;
                }
                else
                {
                    // Accumulate data word bits between sync patterns
                    if (save_data == 1)
                    {
                        data_word_bit_count++;
                        if (data_word_bit_count >= word_len)
                        {
                            if (minor_frame_word_count - 1 < words_in_frame)
                            {
                                frame_words[minor_frame_word_count - 1] = test_word;
                            }
                            data_word_bit_count = 0;
                            minor_frame_word_count++;
                        }

                        if (minor_frame_word_count >= words_in_frame)
                        {
                            save_data = 2;
                        }
                    }
                }
            }

            global_bit_offset += packet_bits;
            total_bytes_processed += raw_len;
        }
    }

    closeFile();

    // Flush the last set of accumulated samples
    if (n_samples > 0)
    {
        writeTimeSample(output, current_time_sample, n_samples, enabled_params);
        rows_written++;
    }

    output.close();
    emit progressUpdated(kPercent100);
    emit logMessage(QString::number(total_bytes_processed) + " bytes processed, "
                    + QString::number(total_syncs_found) + " syncs found, "
                    + QString::number(total_frames_extracted) + " frames extracted.");

    if (total_syncs_found == 0)
    {
        emit errorOccurred("Frame sync pattern was not found in the data stream. "
                           "Verify the frame sync pattern and PCM channel are correct.");
        emit processingFinished(false);
        return false;
    }

    if (total_frames_extracted == 0)
    {
        emit errorOccurred("Frame sync pattern was found but no valid frames were extracted. "
                           "Check the frame parameters and time window settings.");
        emit processingFinished(false);
        return false;
    }

    qint64 elapsed_ms = elapsed_timer.elapsed();
    constexpr double kMsPerSec = 1000.0;
    double elapsed_sec = static_cast<double>(elapsed_ms) / kMsPerSec;
    qint64 output_bytes = QFileInfo(outfile).size();
    constexpr double kMB = 1024.0 * 1024.0;
    constexpr double kKB = 1024.0;
    QString output_size_str = (output_bytes >= static_cast<qint64>(kMB))
        ? QString::number(static_cast<double>(output_bytes) / kMB, 'f', 1) + " MB"
        : QString::number(static_cast<double>(output_bytes) / kKB, 'f', 1) + " KB";

    emit logMessage(QString("Processing complete — %1 rows written, %2, elapsed %3s.")
        .arg(rows_written)
        .arg(output_size_str)
        .arg(elapsed_sec, 0, 'f', 1));

    if (time_gaps_detected > 0)
    {
        emit logMessage(QString("WARNING: %1 time gap(s) detected in recording.").arg(time_gaps_detected));
    }

    emit processingFinished(true);
    return true;
}

void FrameProcessor::writeTimeSample(QFile& output,
                                         double current_time_sample,
                                         int n_samples,
                                         const QVector<ParameterInfo*>& enabled_params)
{
    // add 0.5 ms to time sample so that it rounds up. nicer this way, accounts for floating point imprecision
    double rounded_time = current_time_sample + PCMConstants::kTimeRoundingOffset;
    uint64_t whole_time = static_cast<uint64_t>(rounded_time);
    constexpr double kMillisPerSecond = 1000.0;
    unsigned int millis =
        static_cast<unsigned int>((rounded_time - static_cast<double>(whole_time)) * kMillisPerSecond);

    time_t epoch = static_cast<time_t>(whole_time);
    struct tm* t = gmtime(&epoch);

    // Day-of-year as integer, time as HH:MM:SS.mmm (Excel-compatible)
    constexpr int kBase10 = 10;
    QString row = QString("%1,%2:%3:%4.%5")
        .arg(t->tm_yday + 1)
        .arg(t->tm_hour, 2, kBase10, QChar('0'))
        .arg(t->tm_min, 2, kBase10, QChar('0'))
        .arg(t->tm_sec, 2, kBase10, QChar('0'))
        .arg(millis, 3, kBase10, QChar('0'));
    for (auto* param : enabled_params)
    {
        row += ',' + QString::number(param->sample_sum / n_samples);
        param->sample_sum = 0;
    }
    row += '\n';
    output.write(row.toUtf8());
}
// End of file!
