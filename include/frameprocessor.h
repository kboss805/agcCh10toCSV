/**
 * @file frameprocessor.h
 * @brief Self-contained PCM frame extraction and CSV output processor.
 */

#ifndef FRAMEPROCESSOR_H
#define FRAMEPROCESSOR_H

#include <fstream>
#include <vector>

#include <QObject>
#include <QString>

#include "irig106ch10.h"
#include "i106_time.h"
#include "i106_decode_tmats.h"
#include "i106_decode_pcmf1.h"
#include "i106_decode_time.h"

#include "constants.h"

class FrameSetup;
struct ParameterInfo;

/**
 * @brief Per-channel bookkeeping used by the irig106 C helper layer.
 *
 * Mirrors the irig106utils Hungarian-notation style.
 */
typedef struct _SuChanInfo
{
    uint16_t                uChID;              ///< Channel identifier.
    int                     bEnabled;           ///< Non-zero if channel is enabled.
    Irig106::SuRDataSource* psuRDataSrc;        ///< Pointer to the TMATS R-record data source.
    void*                   psuAttributes;      ///< Pointer to decoded attributes (type varies).
} SuChanInfo;

/**
 * @brief Extracts PCM minor frames from a Chapter 10 file and writes CSV output.
 *
 * Created fresh per processing run, moved to a worker thread, and auto-deleted
 * when the thread finishes. Owns its own irig106 file handle and buffers.
 */
class FrameProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FrameProcessor(QObject* parent = nullptr);
    ~FrameProcessor();

    /**
     * @brief Extracts AGC samples from a Chapter 10 file and writes CSV output.
     *
     * Iterates through all packets, decoding PCM minor frames on the selected
     * channel and averaging samples at the requested rate. Emits
     * progressUpdated() periodically and processingFinished() on completion.
     *
     * @param[in] filename              Path to the .ch10 input file.
     * @param[in] frame_setup           Frame parameter definitions (word map, calibration).
     * @param[in] outfile               Path to the CSV output file.
     * @param[in] time_channel_id       Time channel ID (resolved, not combo box index).
     * @param[in] pcm_channel_id        PCM channel ID (resolved, not combo box index).
     * @param[in] frame_sync            Frame sync pattern as a numeric value.
     * @param[in] sync_pattern_len      Sync pattern length in bits.
     * @param[in] words_in_minor_frame  Words per PCM minor frame (data words + 1).
     * @param[in] bits_in_minor_frame   Total bits per PCM minor frame.
     * @param[in] start_seconds         Start of the extraction window (IRIG seconds).
     * @param[in] stop_seconds          End of the extraction window (IRIG seconds).
     * @param[in] sample_rate           Output sample rate in Hz.
     * @return true if processing completed without errors.
     */
    bool process(const QString& filename,
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
                 int sample_rate);

signals:
    /// Emitted periodically during process() with completion percentage.
    void progressUpdated(int percent);
    /// Emitted when process() finishes; @p success is true on clean completion.
    void processingFinished(bool success);
    /// Emitted at key processing stages with a human-readable status message.
    void logMessage(const QString& message);
    /// Emitted when an error occurs during processing.
    void errorOccurred(const QString& message);

private:
    /// @brief Per-packet timing information for timestamp computation.
    struct PacketTimeRef {
        int64_t base_time;    ///< Packet header reference time (100ns units).
        uint64_t start_bit;   ///< Starting bit position in combined buffer.
        uint64_t num_bits;    ///< Number of data bits from this packet.
    };

    /// @name File I/O helpers
    /// @{
    bool openFile(const QString& filename);
    void closeFile();
    /// @}

    /// @name irig106 C helper wrappers
    /// @{
    /**
     * @brief Deallocates the per-channel info table and associated attributes.
     * @param[in,out] channel_info Array of SuChanInfo pointers to free.
     * @param[in]     max_channels Length of the channel_info array.
     */
    void freeChanInfoTable(SuChanInfo* channel_info[], int max_channels);

    /**
     * @brief Builds per-channel attribute structures from TMATS metadata.
     * @param[in]     tmats_info    Parsed TMATS metadata.
     * @param[in,out] channel_info  Array of SuChanInfo pointers to populate.
     * @param[in]     max_channels  Length of the channel_info array.
     * @return I106_OK on success.
     */
    Irig106::EnI106Status assembleAttributesFromTMATS(
        Irig106::SuTmatsInfo* tmats_info,
        SuChanInfo* channel_info[],
        int max_channels);
    /// @}

    /// @name PCM bit-level helpers
    /// @{
    /**
     * @brief Applies IRIG 106 Appendix D self-synchronizing descrambler.
     * @param[in,out] data       Raw byte buffer to derandomize in-place.
     * @param[in]     total_bits Number of valid bits in the buffer.
     * @param[in,out] lfsr       15-bit LFSR state carried across packets.
     */
    void derandomizeBitstream(uint8_t* data, uint64_t total_bits, uint16_t& lfsr);

    /**
     * @brief Scans a bitstream for the first occurrence of a sync pattern.
     * @param[in] data         Raw byte buffer to scan.
     * @param[in] total_bits   Number of valid bits in the buffer.
     * @param[in] sync_pat     Expected sync pattern value.
     * @param[in] sync_mask    Bitmask for sync pattern comparison.
     * @param[in] sync_pat_len Sync pattern length in bits.
     * @return true if the pattern was found.
     */
    bool hasSyncPattern(const uint8_t* data, uint64_t total_bits,
                        uint64_t sync_pat, uint64_t sync_mask,
                        uint32_t sync_pat_len) const;
    /// @}

    /**
     * @brief Writes one averaged time sample row to the CSV output.
     * @param[in,out] output              Output file stream.
     * @param[in]     current_time_sample Time value for this sample row.
     * @param[in]     n_samples           Number of raw samples to average.
     * @param[in]     enabled_params      Parameter definitions for column output.
     */
    void writeTimeSample(std::ofstream& output,
                         double current_time_sample,
                         int n_samples,
                         const std::vector<ParameterInfo*>& enabled_params);

    Irig106::EnI106Status m_status;                             ///< Last irig106 API return status.
    int m_file_handle;                                          ///< irig106 file handle.
    Irig106::SuI106Ch10Header m_header;                         ///< Reusable packet header buffer.
    unsigned char* m_buffer;                                    ///< Packet data read buffer.
    unsigned long m_buffer_size;                                ///< Current allocated size of m_buffer.
    Irig106::SuTmatsInfo m_tmats_info;                          ///< Parsed TMATS metadata.
    SuChanInfo* m_channel_info[PCMConstants::kMaxChannelCount];  ///< Per-channel attribute table.
    Irig106::SuIrig106Time m_irig_time;                         ///< Reusable IRIG time struct.
    int64_t m_total_file_size;                                  ///< Input file size in bytes (for progress).
};

#endif // FRAMEPROCESSOR_H
