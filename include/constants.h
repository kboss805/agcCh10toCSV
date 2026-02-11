/**
 * @file constants.h
 * @brief Application-wide constants for PCM frame parameters and UI defaults.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

/// @brief Application version information.
struct AppVersion {
    static constexpr int kMajor = 2;   ///< Major version number.
    static constexpr int kMinor = 0;   ///< Minor version number.
    static constexpr int kPatch = 0;   ///< Patch version number.

    /// @return Version string in "major.minor.patch" format.
    static QString toString() { return QString("%1.%2.%3").arg(kMajor).arg(kMinor).arg(kPatch); }
};

/// @brief Constants for PCM frame structure and channel type identifiers.
namespace PCMConstants {
    inline constexpr int kWordsInMinorFrame   = 49;    ///< Words per PCM minor frame.
    inline constexpr int kCommonWordLen        = 16;    ///< Bits per word.
    inline constexpr int kBitsInMinorFrame     = 800;   ///< Total bits per minor frame.
    inline constexpr int kNumMinorFrames       = 1;     ///< Minor frames per major frame.
    inline constexpr int kMaxChannelCount      = 0x10000; ///< Maximum channel ID range.
    inline constexpr const char* kDefaultFrameSync = "FE6B2840"; ///< Default frame sync hex pattern.

    /// Time rounding offset (0.5 ms) used in writeTimeSample.
    inline constexpr double kTimeRoundingOffset = 0.0005;

    /// Maximum raw 16-bit sample value for calibration math.
    inline constexpr uint16_t kMaxRawSampleValue = 0xFFFF;

    /// Default initial buffer size for CH10 packet reading (64 KB).
    inline constexpr unsigned long kDefaultBufferSize = 65536;

    /// Number of packets between progress position queries.
    inline constexpr int kProgressReportInterval = 100;

    /// @name Channel type identifiers from TMATS records
    /// @{
    inline constexpr const char* kChannelTypeTime = "TIMEIN"; ///< TMATS type for time channels.
    inline constexpr const char* kChannelTypePcm  = "PCMIN";  ///< TMATS type for PCM channels.
    /// @}
}

/// @brief Constants for UI configuration, validation limits, and output formatting.
namespace UIConstants {
    /// @name QSettings keys and theme identifiers
    /// @{
    inline constexpr const char* kOrganizationName  = "agcCh10toCSV"; ///< QSettings organization name.
    inline constexpr const char* kApplicationName   = "agcCh10toCSV"; ///< QSettings application name.
    inline constexpr const char* kSettingsKeyTheme  = "Theme";        ///< QSettings key for theme preference.
    inline constexpr const char* kSettingsKeyLastDir = "LastOpenDirectory"; ///< QSettings key for last opened directory.
    inline constexpr const char* kThemeDark         = "dark";         ///< Dark theme identifier.
    inline constexpr const char* kThemeLight        = "light";        ///< Light theme identifier.
    /// @}

    /// @name Receiver grid layout
    /// @{
    inline constexpr int kReceiverGridColumns   = 4;   ///< Number of columns in receiver grid.
    inline constexpr int kTreeItemHeightFactor  = 28;  ///< Approximate height per tree item in pixels.
    inline constexpr int kTreeHeightBuffer      = 10;  ///< Extra height buffer for tree widgets.
    inline constexpr int kTreeFixedWidth        = 100; ///< Fixed width for receiver tree widgets.
    inline constexpr int kLogMinimumWidth       = 300; ///< Minimum width for the log window.
    /// @}

    /// @name Time conversion
    /// @{
    inline constexpr int kSecondsPerDay    = 86400; ///< Seconds in a day.
    inline constexpr int kSecondsPerHour   = 3600;  ///< Seconds in an hour.
    inline constexpr int kSecondsPerMinute = 60;    ///< Seconds in a minute.
    /// @}
    inline constexpr int kDefaultScaleIndex           = 2;     ///< Default voltage scale combo index.
    inline constexpr const char* kDefaultRange        = "100"; ///< Default full-scale range in dB.
    inline constexpr int kDefaultReceiverCount        = 16;    ///< Default number of receivers.
    inline constexpr int kDefaultChannelsPerReceiver  = 3;     ///< Default channels per receiver (L, R, C).
    inline const char* kChannelPrefixes[]             = {"L", "R", "C"}; ///< Channel prefix labels.
    inline constexpr int kNumKnownPrefixes            = 3;     ///< Number of known channel prefixes.

    /// @name Button text
    /// @{
    inline constexpr const char* kButtonTextStart      = "Process";        ///< Process button idle text.
    inline constexpr const char* kButtonTextProcessing = "Processing...";  ///< Process button active text.
    /// @}

    /// @name Time validation limits
    /// @{
    inline constexpr int kMinDayOfYear = 1;   ///< Minimum valid day-of-year.
    inline constexpr int kMaxDayOfYear = 366;  ///< Maximum valid day-of-year.
    inline constexpr int kMaxHour      = 23;   ///< Maximum valid hour.
    inline constexpr int kMaxMinute    = 59;   ///< Maximum valid minute.
    inline constexpr int kMaxSecond    = 59;   ///< Maximum valid second.
    /// @}

    /// @name Sample rate options (Hz)
    /// @{
    inline constexpr int kSampleRate1Hz  = 1;  ///< 1 Hz sample rate.
    inline constexpr int kSampleRate10Hz = 10; ///< 10 Hz sample rate.
    inline constexpr int kSampleRate20Hz = 20; ///< 20 Hz sample rate.
    /// @}

    /// @name Output filename format
    /// @{
    inline constexpr const char* kOutputTimestampFormat = "MMddyyhhmmss"; ///< Timestamp format for output filenames.
    inline constexpr const char* kOutputPrefix          = "output";       ///< Output filename prefix.
    inline constexpr const char* kOutputExtension       = ".csv";         ///< Output file extension.
    /// @}
}

#endif // CONSTANTS_H
