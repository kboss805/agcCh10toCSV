/**
 * @file constants.h
 * @brief Application-wide constants for PCM frame parameters and UI defaults.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/// @brief Constants for PCM frame structure and channel type identifiers.
namespace PCMConstants {
    inline constexpr int kWordsInMinorFrame   = 49;
    inline constexpr int kCommonWordLen        = 16;
    inline constexpr int kBitsInMinorFrame     = 800;
    inline constexpr int kNumMinorFrames       = 1;
    inline constexpr int kMaxChannelCount      = 0x10000;
    inline constexpr const char* kDefaultFrameSync = "FE6B2840";

    // Time rounding offset (0.5 ms) used in writeTimeSample
    inline constexpr double kTimeRoundingOffset = 0.0005;

    // Channel type identifiers from TMATS records
    inline constexpr const char* kChannelTypeTime = "TIMEIN";
    inline constexpr const char* kChannelTypePcm  = "PCMIN";
}

/// @brief Constants for UI configuration, validation limits, and output formatting.
namespace UIConstants {
    inline constexpr int kDefaultScaleIndex           = 2;
    inline constexpr const char* kDefaultRange        = "100";
    inline constexpr int kDefaultReceiverCount        = 16;
    inline constexpr int kDefaultChannelsPerReceiver  = 3;
    inline const char* kChannelPrefixes[]             = {"L", "R", "C"};
    inline constexpr int kNumKnownPrefixes            = 3;

    // Button text
    inline constexpr const char* kButtonTextStart      = "Process";
    inline constexpr const char* kButtonTextProcessing = "Processing...";

    // Time validation limits
    inline constexpr int kMinDayOfYear = 1;
    inline constexpr int kMaxDayOfYear = 366;
    inline constexpr int kMaxHour      = 23;
    inline constexpr int kMaxMinute    = 59;
    inline constexpr int kMaxSecond    = 59;

    // Sample rate options (Hz)
    inline constexpr int kSampleRate1Hz  = 1;
    inline constexpr int kSampleRate10Hz = 10;
    inline constexpr int kSampleRate20Hz = 20;

    // Output filename format
    inline constexpr const char* kOutputTimestampFormat = "MMddyyhhmmss";
    inline constexpr const char* kOutputPrefix          = "output";
    inline constexpr const char* kOutputExtension       = ".csv";
}

#endif // CONSTANTS_H
