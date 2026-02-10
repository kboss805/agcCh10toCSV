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
     * @param[in] filename           Path to the .ch10 input file.
     * @param[in] frame_setup        Frame parameter definitions (word map, calibration).
     * @param[in] outfile            Path to the CSV output file.
     * @param[in] time_channel_id    Time channel ID (resolved, not combo box index).
     * @param[in] pcm_channel_id     PCM channel ID (resolved, not combo box index).
     * @param[in] frame_sync         Frame sync pattern as a numeric value.
     * @param[in] sync_pattern_len   Sync pattern length in bits.
     * @param[in] start_seconds      Start of the extraction window (IRIG seconds).
     * @param[in] stop_seconds       End of the extraction window (IRIG seconds).
     * @param[in] sample_rate        Output sample rate in Hz.
     * @return true if processing completed without errors.
     */
    bool process(const QString& filename,
                 FrameSetup* frame_setup,
                 const QString& outfile,
                 int time_channel_id,
                 int pcm_channel_id,
                 uint64_t frame_sync,
                 int sync_pattern_len,
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
    /**
     * @brief Opens a Chapter 10 file and synchronizes the time reference.
     * @param[in] filename Path to the .ch10 file.
     * @return true on success.
     */
    bool openFile(const QString& filename);

    /// Closes the currently open Chapter 10 file and frees the read buffer.
    void closeFile();

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
