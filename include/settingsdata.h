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
    QString frameSync;        ///< Frame sync pattern as a hex string.
    int polarityIndex;        ///< Polarity combo box index (0=Positive, 1=Negative).
    int slopeIndex;           ///< Voltage slope combo box index.
    QString scale;            ///< Calibration scale in dB/V as a string.
    bool extractAllTime;      ///< True to extract the full time duration.
    int sampleRateIndex;      ///< Sample rate combo box index.
    int receiverCount;        ///< Number of receivers.
    int channelsPerReceiver;  ///< Number of channels per receiver.
};

#endif // SETTINGSDATA_H
