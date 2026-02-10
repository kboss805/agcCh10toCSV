/**
 * @file constants.h
 * @brief Application-wide constants for PCM frame parameters and UI defaults.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

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

    /// @name Channel type identifiers from TMATS records
    /// @{
    inline constexpr const char* kChannelTypeTime = "TIMEIN"; ///< TMATS type for time channels.
    inline constexpr const char* kChannelTypePcm  = "PCMIN";  ///< TMATS type for PCM channels.
    /// @}
}

/// @brief Constants for UI configuration, validation limits, and output formatting.
namespace UIConstants {
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
