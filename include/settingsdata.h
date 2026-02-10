/**
 * @file settingsdata.h
 * @brief Value type for transferring UI settings between MainViewModel and SettingsManager.
 */

#ifndef SETTINGSDATA_H
#define SETTINGSDATA_H

#include <QString>
#include "constants.h"

/**
 * @brief Plain data struct capturing the complete UI configuration state.
 *
 * Used to decouple MainViewModel from SettingsManager without requiring
 * friend-class access. Serialized to/from INI files by SettingsManager.
 */
struct SettingsData
{
    int timeChannelId;        ///< Selected time channel ID.
    int pcmChannelId;         ///< Selected PCM channel ID.
    QString frameSync;        ///< Frame sync pattern as a hex string.
    bool negativePolarity;    ///< True if AGC polarity is negative.
    int scaleIndex;           ///< Voltage scale combo box index.
    QString range;            ///< Full-scale range in dB as a string.
    bool extractAllTime;      ///< True to extract the full time duration.
    int sampleRateIndex;      ///< Sample rate combo box index.
    int receiverCount;        ///< Number of receivers.
    int channelsPerReceiver;  ///< Number of channels per receiver.
};

#endif // SETTINGSDATA_H
