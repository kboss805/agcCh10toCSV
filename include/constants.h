/**
 * @file constants.h
 * @brief Application-wide constants for PCM frame parameters and UI defaults.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <array>
#include <cstdint>
#include <QColor>
#include <QString>

/// @brief Application version information.
struct AppVersion {
    static constexpr int kMajor = 3;   ///< Major version number.
    static constexpr int kMinor = 1;   ///< Minor version number.
    static constexpr int kPatch = 2;   ///< Patch version number.

    /// @return Version string in "major.minor.patch" format.
    static QString toString() { return QString("%1.%2.%3").arg(kMajor).arg(kMinor).arg(kPatch); }
};

/// @brief Constants for PCM frame structure and channel type identifiers.
namespace PCMConstants {
    inline constexpr int kCommonWordLen        = 16;    ///< Bits per word.
    inline constexpr int kSecondsPerMinute     = 60;    ///< Seconds in a minute.
    inline constexpr int kMinutesPerHour       = 60;    ///< Minutes in an hour.
    inline constexpr int kHoursPerDay          = 24;    ///< Hours in a day.
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

    /// Default number of PCM packets to scan during pre-scan encoding detection.
    inline constexpr int kPreScanMaxPackets = 5;

    /// @name Channel type identifiers from TMATS records
    /// @{
    inline constexpr const char* kChannelTypeTime = "TIMEIN"; ///< TMATS type for time channels.
    inline constexpr const char* kChannelTypePcm  = "PCMIN";  ///< TMATS type for PCM channels.
    /// @}

    /// Regex pattern for validating hexadecimal input strings (e.g., frame sync).
    inline constexpr const char* kFrameSyncHexPattern = "^[0-9A-Fa-f]+$";
}

/// @brief Constants for UI configuration, validation limits, and output formatting.
namespace UIConstants {
    /// @name QSettings keys and theme identifiers
    /// @{
    inline constexpr const char* kOrganizationName  = "agcCh10toCSV"; ///< QSettings organization name.
    inline constexpr const char* kApplicationName   = "agcCh10toCSV"; ///< QSettings application name.
    inline constexpr const char* kSettingsKeyTheme  = "Theme";        ///< QSettings key for theme preference.
    inline constexpr const char* kSettingsKeyLastCh10Dir = "LastCh10Directory"; ///< QSettings key for last Ch10 file dialog directory.
    inline constexpr const char* kSettingsKeyLastCsvDir  = "LastCsvDirectory";  ///< QSettings key for last CSV file dialog directory.
    inline constexpr const char* kSettingsKeyLastIniDir  = "LastIniDirectory";  ///< QSettings key for last INI file dialog directory.
    inline constexpr const char* kThemeDark         = "dark";         ///< Dark theme identifier.
    inline constexpr const char* kThemeLight        = "light";        ///< Light theme identifier.
    inline constexpr const char* kSettingsKeyRecentFiles = "RecentFiles"; ///< QSettings key for recent files list.
    inline constexpr int kMaxRecentFiles            = 5;              ///< Maximum number of recent files to remember.
    inline constexpr const char* kSettingsKeyPlotVisible = "PlotVisible"; ///< QSettings key for plot dock visibility.
    /// @}

    /// @name Receiver grid layout
    /// @{
    inline constexpr int kReceiverGridColumns   = 4;   ///< Number of columns in receiver grid.
    inline constexpr int kTreeItemHeightFactor  = 24;  ///< Approximate height per tree item in pixels.
    inline constexpr int kTreeHeightBuffer      = 4;   ///< Extra height buffer for tree widgets.
    inline constexpr int kTreeFixedWidth        = 100; ///< Fixed width for receiver tree widgets.
    inline constexpr int kFlatButtonMinWidth    = 90;  ///< Minimum width for flat action buttons (accommodates "Collapse All").
    inline constexpr int kLogMinimumWidth       = 400; ///< Minimum width for the log window.
    inline constexpr int kLogPreviewHeight      = 80;  ///< Fixed height for the log preview panel.
    /// @}

    /// @name Time conversion
    /// @{
    inline constexpr int kSecondsPerDay    = 86400; ///< Seconds in a day.
    inline constexpr int kSecondsPerHour   = 3600;  ///< Seconds in an hour.
    inline constexpr int kSecondsPerMinute = 60;    ///< Seconds in a minute.
    /// @}
    inline constexpr int kDefaultSlopeIndex           = 3;     ///< Default voltage slope combo index.
    inline constexpr int kMaxSlopeIndex               = 3;     ///< Maximum valid voltage slope combo index.
    inline constexpr int kMaxSampleRateIndex           = 2;     ///< Maximum valid sample rate combo index.
    inline constexpr const char* kDefaultScale        = "20";  ///< Default calibration scale in dB per volt.
    inline constexpr int kDefaultReceiverCount        = 16;    ///< Default number of receivers.
    inline constexpr int kMinReceiverCount            = 1;     ///< Minimum valid number of receivers.
    inline constexpr int kMaxReceiverCount            = 16;    ///< Maximum valid number of receivers.
    inline constexpr int kDefaultChannelsPerReceiver  = 3;     ///< Default channels per receiver (L, R, C).
    inline constexpr int kMinChannelsPerReceiver      = 1;     ///< Minimum valid channels per receiver.
    inline constexpr int kMaxChannelsPerReceiver      = 48;    ///< Maximum valid channels per receiver.
    inline constexpr int kMaxTotalParameters          = 48;    ///< Maximum total parameter words (receivers x channels).
    inline constexpr std::array<const char*, 3> kChannelPrefixes = {"L", "R", "C"}; ///< Channel prefix labels.
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
    inline constexpr int kSampleRate1Hz   = 1;   ///< 1 Hz sample rate.
    inline constexpr int kSampleRate10Hz  = 10;  ///< 10 Hz sample rate.
    inline constexpr int kSampleRate100Hz = 100; ///< 100 Hz sample rate.
    inline constexpr int kDefaultSampleRateIndex = 1; ///< Default sample rate combo index (10 Hz).
    /// @}

    /// @name Voltage scale bounds (indexed by scale combo box)
    /// @{
    inline constexpr std::array<double, 4> kSlopeVoltageLower = {-10.0, -5.0, 0.0, 0.0}; ///< Lower voltage bound per scale option.
    inline constexpr std::array<double, 4> kSlopeVoltageUpper = {10.0, 5.0, 10.0, 5.0};  ///< Upper voltage bound per scale option.
    /// @}

    /// @name Display labels for combo box indices
    /// @{
    inline constexpr std::array<const char*, 4> kSlopeLabels     = {"+/-10V", "+/-5V", "0-10V", "0-5V"};          ///< Voltage slope display labels.
    inline constexpr std::array<const char*, 3> kSampleRateLabels = {"1 Hz", "10 Hz", "100 Hz"};         ///< Sample rate display labels.
    /// @}

    /// @name Polarity combo box
    /// @{
    inline constexpr int kDefaultPolarityIndex = 1;  ///< Default polarity combo index (Negative).
    inline constexpr int kMaxPolarityIndex     = 1;  ///< Maximum valid polarity combo index.
    inline constexpr std::array<const char*, 2> kPolarityLabels  = {"Positive", "Negative"}; ///< Polarity display labels.
    /// @}

    /// @name Output filename format
    /// @{
    inline constexpr const char* kOutputTimestampFormat = "MMddyyhhmmss"; ///< Timestamp format for output filenames.
    inline constexpr const char* kOutputPrefix          = "output";       ///< Output filename prefix.
    inline constexpr const char* kOutputExtension       = ".csv";         ///< Output file extension.
    /// @}

    /// @name Batch processing
    /// @{
    inline constexpr const char* kBatchOutputPrefix      = "AGC_";                       ///< Output filename prefix for batch mode.
    inline constexpr const char* kSettingsKeyLastBatchDir = "LastBatchOutputDirectory";   ///< QSettings key for last batch output directory.
    inline constexpr int kBatchFileListHeight            = 180;                          ///< Fixed height for file list tree (px).
    inline constexpr int kProgressBarMax                 = 100;                          ///< Maximum value for the progress bar.
    inline constexpr int kTreeIndentation                = 12;                           ///< Indentation width for tree widgets (px).
    inline constexpr int kLayoutSpacingSmall             = 8;                            ///< Small layout spacing (px).
    inline constexpr int kLayoutSpacingLarge             = 16;                           ///< Large layout spacing (px).
    inline constexpr int kToolbarIconSize                = 24;                           ///< Toolbar icon size (px).
    inline constexpr int kAboutIconSize                  = 64;                           ///< About dialog icon size (px).
    inline constexpr int kLogDialogWidth                 = 600;                          ///< Default log dialog width (px).
    inline constexpr int kLogDialogHeight                = 400;                          ///< Default log dialog height (px).
    inline constexpr int kTimeInputMaxWidth               = 100;                          ///< Maximum width for time input fields (px).
    inline constexpr int kChannelComboFixedWidth           = 400;                          ///< Fixed width for Time/PCM channel combo boxes (px).
    inline constexpr int kFileNameColumnMinWidth           = 600;                          ///< Minimum width for the file name column in the file list tree (px).
    inline constexpr int kControlsDockMinWidth            = 600;                          ///< Minimum width for the controls dock panel (file name column + margins).
    inline constexpr int kDecimalBase                    = 10;                           ///< Decimal (base-10) radix for QString::arg formatting.
    inline constexpr int kHexBase                        = 16;                           ///< Hexadecimal (base-16) radix for string parsing.
    inline constexpr int kBytesPerKB                     = 1024;                         ///< Bytes per kilobyte.
    inline constexpr int kBytesPerMB                     = 1048576;                      ///< Bytes per megabyte.
    /// @}

    /// @name Deployment / portable mode
    /// @{
    inline constexpr const char* kPortableMarkerFilename = "portable";    ///< Marker file name for portable mode detection.
    inline constexpr const char* kSettingsDirName         = "settings";   ///< Settings directory name relative to app root.
    inline constexpr const char* kDefaultIniFilename      = "default.ini"; ///< Default INI configuration filename.
    /// @}
}

/// @brief Constants for the AGC signal plot window.
namespace PlotConstants {
    inline constexpr int kPlotDockMinWidth   = 500;   ///< Minimum plot dock width in pixels.
    inline constexpr int kPlotDockMinHeight  = 300;   ///< Minimum plot dock height in pixels.
    inline constexpr double kAxisMarginFactor = 0.05; ///< Y-axis padding as fraction of data range.
    inline constexpr const char* kDefaultPlotTitle = "AGC Signal Plot"; ///< Default plot title.
    inline constexpr const char* kYAxisLabel = "Amplitude (dB)";       ///< Y-axis label.
    inline constexpr const char* kXAxisLabel = "Time (DDD:HH:MM:SS)";             ///< X-axis label.
    inline constexpr double kZoomFactor      = 0.1;   ///< Wheel zoom step (10% per notch).

    /// @name Theme colors
    /// @{
    inline constexpr QColor kDarkBackground  {32, 32, 32};       ///< Dark theme chart background.
    inline constexpr QColor kLightBackground {255, 255, 255};    ///< Light theme chart background.
    inline constexpr QColor kDarkForeground  {220, 220, 220};    ///< Dark theme axis/text color.
    inline constexpr QColor kLightForeground {30, 30, 30};       ///< Light theme axis/text color.
    inline constexpr QColor kDarkGridColor   {60, 60, 60};       ///< Dark theme grid line color.
    inline constexpr QColor kLightGridColor  {200, 200, 200};    ///< Light theme grid line color.
    /// @}

    /// @name Plot widget parameters
    /// @{
    inline constexpr int kTickCount          = 10;               ///< Number of major tick marks on X axis.
    inline constexpr double kGraphPenWidth   = 1.5;              ///< Width of series graph pen.
    inline constexpr int kTitleFontSize      = 10;               ///< Plot title font size in points.
    inline constexpr double kSpinBoxMaxRange = 1e9;              ///< Maximum range for X axis spinboxes.
    inline constexpr double kYSpinBoxMax     = 999.0;            ///< Maximum range for Y axis spinboxes.
    /// @}

    /// @brief Base color palette for receiver series (one hue per receiver).
    inline constexpr int kNumReceiverColors = 10;
    inline constexpr std::array<QColor, kNumReceiverColors> kReceiverColors = {
        QColor(31, 119, 180),   ///< Blue
        QColor(255, 127, 14),   ///< Orange
        QColor(44, 160, 44),    ///< Green
        QColor(214, 39, 40),    ///< Red
        QColor(148, 103, 189),  ///< Purple
        QColor(140, 86, 75),    ///< Brown
        QColor(227, 119, 194),  ///< Pink
        QColor(127, 127, 127),  ///< Gray
        QColor(188, 189, 34),   ///< Olive
        QColor(23, 190, 207),   ///< Cyan
    };
}

#endif // CONSTANTS_H
