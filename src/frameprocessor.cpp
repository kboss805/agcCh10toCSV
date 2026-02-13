/**
 * @file frameprocessor.cpp
 * @brief Implementation of FrameProcessor — PCM frame extraction and CSV output.
 */

#include "frameprocessor.h"

#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include <QDebug>
#include <QFileInfo>

#include "constants.h"
#include "framesetup.h"

using namespace Irig106;

////////////////////////////////////////////////////////////////////////////////
//                          IRIG106 HELPER METHODS                            //
////////////////////////////////////////////////////////////////////////////////

void FrameProcessor::freeChanInfoTable(SuChanInfo* apsuChanInfo[], int MaxSuChanInfo)
{
    if(apsuChanInfo == NULL)
        return;

    for(int iTrackNumber = 0; iTrackNumber < MaxSuChanInfo; iTrackNumber++)
    {
        if(apsuChanInfo[iTrackNumber] != NULL)
        {
            if(apsuChanInfo[iTrackNumber]->psuAttributes != NULL)
            {
                if (strcasecmp(apsuChanInfo[iTrackNumber]->psuRDataSrc->szChannelDataType,"PCMIN") == 0)
                    FreeOutputBuffers_PcmF1((SuPcmF1_Attributes *) apsuChanInfo[iTrackNumber]->psuAttributes);
                free(apsuChanInfo[iTrackNumber]->psuAttributes);
                apsuChanInfo[iTrackNumber]->psuAttributes = NULL;
            }

            free(apsuChanInfo[iTrackNumber]);
            apsuChanInfo[iTrackNumber] = NULL;
        }
    }
}

EnI106Status FrameProcessor::assembleAttributesFromTMATS(SuTmatsInfo* psuTmatsInfo,
                                                         SuChanInfo* apsuChanInfo[],
                                                         int MaxSuChanInfo)
{
    static const char* szModuleText = "Assemble Attributes From TMATS";
    char szText[_MAX_PATH + _MAX_PATH];
    int SizeOfText = sizeof(szText);
    int TextLen = 0;

    SuRRecord* psuRRecord;
    SuRDataSource* psuRDataSrc;
    int iTrackNumber;
    memset(szText, 0, SizeOfText--);

    if((psuTmatsInfo->psuFirstGRecord == NULL) || (psuTmatsInfo->psuFirstRRecord == NULL))
    {
        _snprintf(&szText[TextLen], SizeOfText - TextLen, "%s: %s\n", szModuleText, szI106ErrorStr(I106_INVALID_DATA));
        return(I106_INVALID_DATA);
    }

    psuRRecord = psuTmatsInfo->psuFirstRRecord;
    while (psuRRecord != NULL)
    {
        psuRDataSrc = psuRRecord->psuFirstDataSource;
        while (psuRDataSrc != NULL)
        {
            if(psuRDataSrc->szTrackNumber == NULL)
                continue;

            iTrackNumber = atoi(psuRDataSrc->szTrackNumber);

            if(iTrackNumber >= MaxSuChanInfo)
                return(I106_BUFFER_TOO_SMALL);

            if (apsuChanInfo[iTrackNumber] == NULL)
            {
                if((apsuChanInfo[iTrackNumber] = (SuChanInfo *)calloc(1, sizeof(SuChanInfo))) == NULL)
                {
                    _snprintf(&szText[TextLen], SizeOfText - TextLen, "%s: %s\n", szModuleText, szI106ErrorStr(I106_BUFFER_TOO_SMALL));
                    freeChanInfoTable(apsuChanInfo, MaxSuChanInfo);
                    return(I106_BUFFER_TOO_SMALL);
                }

                apsuChanInfo[iTrackNumber]->uChID = iTrackNumber;
                apsuChanInfo[iTrackNumber]->bEnabled = psuRDataSrc->szEnabled[0] == 'T';
                apsuChanInfo[iTrackNumber]->psuRDataSrc = psuRDataSrc;

                if (strcasecmp(psuRDataSrc->szChannelDataType,"PCMIN") == 0)
                {
                    if((apsuChanInfo[iTrackNumber]->psuAttributes = calloc(1, sizeof(SuPcmF1_Attributes))) == NULL)
                    {
                        _snprintf(&szText[TextLen], SizeOfText - TextLen, "%s: %s\n", szModuleText, szI106ErrorStr(I106_BUFFER_TOO_SMALL));
                        freeChanInfoTable(apsuChanInfo, MaxSuChanInfo);
                        return(I106_BUFFER_TOO_SMALL);
                    }
                    (void)Set_Attributes_PcmF1(psuRDataSrc, (SuPcmF1_Attributes *)apsuChanInfo[iTrackNumber]->psuAttributes);
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

void FrameProcessor::derandomizeBitstream(uint8_t* data, uint64_t total_bits, uint16_t& lfsr)
{
    for (uint64_t i = 0; i < total_bits; i++)
    {
        uint32_t byte_idx = static_cast<uint32_t>(i >> 3);
        uint8_t bit_mask = 0x80 >> (i & 7);

        uint8_t received_bit = (data[byte_idx] & bit_mask) ? 1 : 0;
        uint8_t descrambled = received_bit ^ ((lfsr >> 13) & 1) ^ ((lfsr >> 14) & 1);
        lfsr = ((lfsr << 1) | received_bit) & 0x7FFF;

        if (descrambled)
            data[byte_idx] |= bit_mask;
        else
            data[byte_idx] &= ~bit_mask;
    }
}

bool FrameProcessor::hasSyncPattern(const uint8_t* data, uint64_t total_bits,
                                    uint64_t sync_pat, uint64_t sync_mask,
                                    uint32_t sync_pat_len) const
{
    uint64_t test_word = 0;
    uint64_t bits_loaded = 0;
    for (uint64_t i = 0; i < total_bits; i++)
    {
        uint32_t byte_idx = static_cast<uint32_t>(i >> 3);
        uint8_t bit_val = (data[byte_idx] & (0x80 >> (i & 7))) ? 1 : 0;
        test_word = (test_word << 1) | bit_val;
        bits_loaded++;
        if (bits_loaded >= sync_pat_len &&
            (test_word & sync_mask) == sync_pat)
            return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
//                       CONSTRUCTOR / DESTRUCTOR                             //
////////////////////////////////////////////////////////////////////////////////

FrameProcessor::FrameProcessor(QObject* parent)
    : QObject(parent),
      m_buffer(nullptr),
      m_buffer_size(0L),
      m_total_file_size(0)
{
    putenv("TZ=GMT0");
    tzset();
    memset(m_channel_info, 0, sizeof(m_channel_info));

    m_buffer = (unsigned char*)malloc(PCMConstants::kDefaultBufferSize);
    if (m_buffer)
        m_buffer_size = PCMConstants::kDefaultBufferSize;
}

FrameProcessor::~FrameProcessor()
{
    freeChanInfoTable(m_channel_info, PCMConstants::kMaxChannelCount);
    if (m_buffer)
        free(m_buffer);
}

////////////////////////////////////////////////////////////////////////////////
//                            FILE I/O                                        //
////////////////////////////////////////////////////////////////////////////////

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

void FrameProcessor::closeFile()
{
    enI106Ch10Close(m_file_handle);
    if (m_buffer)
        free(m_buffer);
    m_buffer = nullptr;
    m_buffer_size = 0L;
}

////////////////////////////////////////////////////////////////////////////////
//                          PROCESSING                                        //
////////////////////////////////////////////////////////////////////////////////

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
                                int sample_rate)
{
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

    memset(m_channel_info, 0, sizeof(m_channel_info));

    // Open input file and sync time
    emit logMessage("Opening Chapter 10 file...");
    if (!openFile(filename))
    {
        emit errorOccurred("Failed to load Chapter 10 file.");
        emit processingFinished(false);
        return false;
    }

    // Open output file with larger stream buffer for performance
    emit logMessage("Creating output CSV file...");
    char stream_buffer[65536];
    std::ofstream output;
    output.rdbuf()->pubsetbuf(stream_buffer, sizeof(stream_buffer));
    output.open(outfile.toUtf8().constData());
    if (!output.is_open())
    {
        emit errorOccurred("Failed to open output file: " + outfile);
        closeFile();
        emit processingFinished(false);
        return false;
    }

    // Cleanup helper for error paths
    auto fail = [&](const QString& msg) -> bool {
        emit errorOccurred(msg);
        if (output.is_open()) output.close();
        closeFile();
        emit processingFinished(false);
        return false;
    };

    // Pre-cache enabled parameters to avoid repeated iteration in hot loops
    std::vector<ParameterInfo*> enabled_params;
    enabled_params.reserve(frame_setup->length());
    for (int i = 0; i < frame_setup->length(); i++)
    {
        ParameterInfo* param = frame_setup->getParameter(i);
        if (param->is_enabled)
            enabled_params.push_back(param);
    }

    // Write CSV header
    output << "Day,Time";
    for (const auto* param : enabled_params)
        output << "," << param->name.toUtf8().constData();
    output << "\n";

    // Read and process the first packet (must be TMATS)
    emit logMessage("Reading TMATS metadata...");
    m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);

    if (m_status != I106_OK)
        return fail("Failed to read first header.");

    if (m_header.ubyDataType == I106CH10_DTYPE_TMATS)
    {
        if (m_buffer_size < m_header.ulPacketLen)
        {
            unsigned char* new_buffer = (unsigned char*) realloc(m_buffer, m_header.ulPacketLen);
            if (!new_buffer)
                return fail("Memory allocation failed.");
            m_buffer = new_buffer;
            m_buffer_size = m_header.ulPacketLen;
        }

        m_status = enI106Ch10ReadData(m_file_handle, m_buffer_size, m_buffer);
        if (m_status != I106_OK)
            return fail("Failed to read data from first header.");

        memset(&m_tmats_info, 0, sizeof(m_tmats_info));
        m_status = enI106_Decode_Tmats(&m_header, m_buffer, &m_tmats_info);
        if (m_status != I106_OK)
            return fail("Failed to process TMATS info from first header.");

        m_status = assembleAttributesFromTMATS(&m_tmats_info, m_channel_info, PCMConstants::kMaxChannelCount);
        if (m_status != I106_OK)
            return fail("Failed to assemble attributes from TMATS header.");
    }
    else
    {
        return fail("Failed to find TMATS message.");
    }

    // Set up PCM attributes for the selected channel
    emit logMessage("Setting up PCM attributes...");
    if (m_channel_info[pcm_channel_id] == nullptr)
        return fail("Channel info not set up for selected PCM channel.");

    SuPcmF1_Attributes* pcm_attrs = (SuPcmF1_Attributes*) m_channel_info[pcm_channel_id]->psuAttributes;
    if (pcm_attrs == nullptr)
        return fail("Unable to load PCM attributes.");

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
                              frame_sync, // llMinorFrameSyncPat
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

    std::vector<uint64_t> frame_words(words_in_frame, 0);

    // CSV output state
    double sample_period = 1.0 / sample_rate;
    double current_time_sample = start_seconds;
    double next_time_sample = current_time_sample + sample_period;
    int n_samples = 0;

    for (auto* param : enabled_params)
        param->sample_sum = 0;

    // Timestamp tracking: keep current and previous packet time references
    uint64_t global_bit_offset = 0;
    PacketTimeRef current_time_ref = {0, 0, 0};
    PacketTimeRef prev_time_ref = {0, 0, 0};
    bool has_time_ref = false;

    // Derandomization state (auto-detected on first PCM packet)
    bool needs_derand = false;
    bool derand_decided = false;
    uint16_t lfsr_state = 0;

    // -----------------------------------------------------------------------
    // Single pass: read packets and process PCM data immediately
    // -----------------------------------------------------------------------
    emit logMessage("Processing PCM data...");
    int packet_count = 0;

    while (true)
    {
        m_status = enI106Ch10ReadNextHeader(m_file_handle, &m_header);
        if (m_status == I106_EOF)
            break;
        if (m_status != I106_OK)
        {
            emit errorOccurred("File read error during data collection.");
            break;
        }

        // Report progress every N packets to reduce I/O overhead
        packet_count++;
        if (m_total_file_size > 0 && (packet_count % PCMConstants::kProgressReportInterval) == 0)
        {
            int64_t current_pos = 0;
            enI106Ch10GetPos(m_file_handle, &current_pos);
            int percent = static_cast<int>(current_pos * 100 / m_total_file_size);
            if (percent != last_reported_percent)
            {
                if (percent / 10 != last_reported_percent / 10 && percent > 0)
                    emit logMessage(QString::number(percent) + "% complete...");
                last_reported_percent = percent;
                emit progressUpdated(percent);
            }
        }

        // Process IRIG time packets to maintain time sync
        if (m_header.ubyDataType == I106CH10_DTYPE_IRIG_TIME && m_header.uChID == time_channel_id)
        {
            if (m_buffer_size < m_header.ulPacketLen)
            {
                unsigned char* new_buffer = (unsigned char*) realloc(m_buffer, m_header.ulPacketLen);
                if (!new_buffer)
                {
                    emit errorOccurred("Memory allocation failed.");
                    break;
                }
                m_buffer = new_buffer;
                m_buffer_size = m_header.ulPacketLen;
            }

            m_status = enI106Ch10ReadData(m_file_handle, m_buffer_size, m_buffer);
            if (m_status != I106_OK)
            {
                emit errorOccurred("File read error; aborting parsing.");
                break;
            }

            enI106_Decode_TimeF1(&m_header, m_buffer, &m_irig_time);
            enI106_SetRelTime(m_file_handle, &m_irig_time, m_header.aubyRefTime);
        }

        // Process PCM data from the selected channel
        if (m_header.ubyDataType == I106CH10_DTYPE_PCM_FMT_1 && m_header.uChID == pcm_channel_id)
        {
            if (m_buffer_size < m_header.ulPacketLen)
            {
                unsigned char* new_buffer = (unsigned char*) realloc(m_buffer, m_header.ulPacketLen);
                if (!new_buffer)
                {
                    emit errorOccurred("Memory allocation failed.");
                    break;
                }
                m_buffer = new_buffer;
                m_buffer_size = m_header.ulPacketLen;
            }

            m_status = enI106Ch10ReadData(m_file_handle, m_buffer_size, m_buffer);
            if (m_status != I106_OK)
            {
                emit errorOccurred("File read error; aborting parsing.");
                break;
            }

            // Skip the 4-byte SuPcmF1_ChanSpec header to get raw PCM data
            uint32_t data_offset = sizeof(SuPcmF1_ChanSpec);
            if (m_header.ulDataLen <= data_offset)
                continue;

            uint8_t* raw_data = m_buffer + data_offset;
            uint32_t raw_len = m_header.ulDataLen - data_offset;

            // Byte-swap raw data if needed (library default: swap)
            if (!pcm_attrs->bDontSwapRawData)
                SwapBytes_PcmF1(raw_data, raw_len);

            uint64_t packet_bits = static_cast<uint64_t>(raw_len) * 8;

            // Auto-detect derandomization on the first PCM packet
            if (!derand_decided)
            {
                if (hasSyncPattern(raw_data, packet_bits, sync_pat, sync_mask, sync_pat_len))
                {
                    emit logMessage("Frame sync detected in raw data.");
                    needs_derand = false;
                }
                else
                {
                    emit logMessage("Sync not found; derandomizing bitstream...");
                    needs_derand = true;
                    derandomizeBitstream(raw_data, packet_bits, lfsr_state);
                }
                derand_decided = true;
            }
            else if (needs_derand)
            {
                derandomizeBitstream(raw_data, packet_bits, lfsr_state);
            }

            // Update time references (keep current + previous for boundary frames)
            int64_t pkt_base_time = 0;
            vTimeArray2LLInt(m_header.aubyRefTime, &pkt_base_time);

            if (has_time_ref)
                prev_time_ref = current_time_ref;

            current_time_ref.base_time = pkt_base_time;
            current_time_ref.start_bit = global_bit_offset;
            current_time_ref.num_bits = packet_bits;
            has_time_ref = true;

            // Process all bits in this packet through the frame extraction state machine
            for (uint64_t bit_pos = 0; bit_pos < packet_bits; bit_pos++)
            {
                uint32_t byte_idx = static_cast<uint32_t>(bit_pos >> 3);
                uint8_t bit_val = (raw_data[byte_idx] & (0x80 >> (bit_pos & 7))) ? 1 : 0;

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
                            // Compute the time for this frame
                            uint64_t global_bit_pos = global_bit_offset + bit_pos;
                            uint64_t frame_start_bit = global_bit_pos + 1 - bits_in_frame;

                            // Find the correct time reference (current or previous packet)
                            const PacketTimeRef& ref =
                                (frame_start_bit >= current_time_ref.start_bit)
                                    ? current_time_ref : prev_time_ref;

                            int64_t frame_rel_time = ref.base_time +
                                static_cast<int64_t>((frame_start_bit - ref.start_bit) * delta_100ns);

                            enI106_RelInt2IrigTime(m_file_handle, frame_rel_time, &m_irig_time);
                            double current_time = 0.0000001 * m_irig_time.ulFrac + m_irig_time.ulSecs;

                            if (current_time >= start_seconds && current_time <= stop_seconds)
                            {
                                if (next_time_sample < current_time)
                                {
                                    if (n_samples > 0)
                                        writeTimeSample(output, current_time_sample, n_samples, enabled_params);

                                    n_samples = 0;
                                    do
                                    {
                                        current_time_sample += sample_period;
                                        next_time_sample += sample_period;
                                    } while (next_time_sample < current_time);
                                }

                                for (auto* param : enabled_params)
                                {
                                    if (param->word >= 0 &&
                                        param->word < static_cast<int>(words_in_frame))
                                    {
                                        int64_t raw_value = static_cast<int64_t>(frame_words[param->word] & word_mask);
                                        double scaled_value = (raw_value + param->scale) * param->slope;
                                        param->sample_sum += scaled_value;
                                    }
                                }

                                n_samples++;
                                total_frames_extracted++;
                            }
                        }

                        minor_frame_bit_count = 0;
                        minor_frame_word_count = 1;
                        data_word_bit_count = 0;
                        save_data = 1;
                    }
                    else
                    {
                        sync_count = 0;
                        minor_frame_bit_count = 0;
                        minor_frame_word_count = 1;
                        data_word_bit_count = 0;
                        save_data = 1;
                    }

                    continue;
                }

                // Collect data word bits
                if (save_data == 1)
                {
                    data_word_bit_count++;
                    if (data_word_bit_count >= word_len)
                    {
                        if (minor_frame_word_count - 1 < words_in_frame)
                            frame_words[minor_frame_word_count - 1] = test_word;
                        data_word_bit_count = 0;
                        minor_frame_word_count++;
                    }
                }

                if (minor_frame_word_count >= words_in_frame)
                    save_data = 2;
            }

            global_bit_offset += packet_bits;
            total_bytes_processed += raw_len;
        }
    }

    closeFile();

    // Flush the last set of accumulated samples
    if (n_samples > 0)
        writeTimeSample(output, current_time_sample, n_samples, enabled_params);

    output.close();
    emit progressUpdated(100);
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

    emit logMessage("Processing complete.");
    emit processingFinished(true);
    return true;
}

void FrameProcessor::writeTimeSample(std::ofstream& output,
                                         double current_time_sample,
                                         int n_samples,
                                         const std::vector<ParameterInfo*>& enabled_params)
{
    // add 0.5 ms to time sample so that it rounds up. nicer this way, accounts for floating point imprecision
    double rounded_time = current_time_sample + PCMConstants::kTimeRoundingOffset;
    uint64_t whole_time = (uint64_t) rounded_time;
    unsigned int millis =
        (unsigned int)((rounded_time - (double) whole_time) * 1000);

    time_t epoch = (time_t) whole_time;
    struct tm* t = gmtime(&epoch);

    // Day-of-year as integer, time as HH:MM:SS.mmm (Excel-compatible)
    char time_buf[30];
    snprintf(time_buf, sizeof(time_buf), "%d,%02d:%02d:%02d.%03u",
             t->tm_yday + 1,
             t->tm_hour, t->tm_min, t->tm_sec, millis);
    std::string row(time_buf);
    for (auto* param : enabled_params)
    {
        row += ',';
        row += std::to_string(param->sample_sum / n_samples);
        param->sample_sum = 0;
    }
    row += '\n';
    output.write(row.data(), row.size());
}
