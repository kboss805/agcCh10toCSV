/**
 * @file batchfileinfo.h
 * @brief Per-file metadata for batch processing.
 */

#ifndef BATCHFILEINFO_H
#define BATCHFILEINFO_H

#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief Stores metadata and validation state for one file in a batch.
 *
 * Populated during batch file loading (channel discovery) and updated
 * during validation and pre-scan phases. Plain value type following the
 * same pattern as SettingsData.
 */
struct BatchFileInfo
{
    QString filepath;               ///< Absolute path to the .ch10 file.
    QString filename;               ///< Base filename (QFileInfo::fileName()).
    qint64  fileSize = 0;           ///< File size in bytes.

    /// @name Channel discovery results
    /// @{
    QStringList pcmChannelStrings;  ///< PCM channel display strings from this file.
    QStringList timeChannelStrings; ///< Time channel display strings from this file.
    QVector<int> pcmChannelIds;     ///< PCM channel IDs corresponding to pcmChannelStrings.
    /// @}

    /// @name Validation state (updated on channel change)
    /// @{
    bool hasPcmChannel  = false;    ///< True if a usable PCM channel was resolved for this file.
    bool hasTimeChannel = false;    ///< True if a usable time channel was resolved for this file.
    int resolvedPcmIndex  = -1;    ///< Index into this file's pcmChannelStrings (0-based), or -1.
    int resolvedTimeIndex = -1;    ///< Index into this file's timeChannelStrings (0-based), or -1.
    bool preScanOk      = false;    ///< True if sync pattern was found during pre-scan.
    bool isRandomized   = false;    ///< True if RNRZ-L encoding detected.
    bool skip           = false;    ///< True if file should be skipped during processing.
    QString skipReason;             ///< Human-readable reason if skip is true.
    /// @}

    /// @name Processing state
    /// @{
    bool processed   = false;       ///< True if processing has been attempted.
    bool processedOk = false;       ///< True if processing completed successfully.
    QString outputFile;             ///< Path to the generated CSV output file.
    /// @}
};

#endif // BATCHFILEINFO_H
