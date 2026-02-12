/**
 * @file settingsmanager.cpp
 * @brief Implementation of SettingsManager — INI file persistence.
 */

#include "settingsmanager.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

#include "constants.h"
#include "framesetup.h"
#include "mainviewmodel.h"

SettingsManager::SettingsManager(MainViewModel* view_model) :
    m_view_model(view_model)
{

}

void SettingsManager::loadFile(const QString& filename)
{
    static const QRegularExpression hex_pattern("^[0-9A-Fa-f]+$");

    emit logMessage("Loading settings: " + QFileInfo(filename).fileName());

    QSettings loaded_settings(filename, QSettings::IniFormat);
    if (loaded_settings.status() != QSettings::NoError)
    {
        emit logMessage("  ERROR: Could not read file.");
        return;
    }

    SettingsData data;

    // --- Channels ---
    loaded_settings.beginGroup("Channels");
    data.timeChannelId = loaded_settings.value("TimeChannel").toInt();
    data.pcmChannelId = loaded_settings.value("PCMChannel").toInt();
    loaded_settings.endGroup();

    // --- Frame sync (hex string) ---
    loaded_settings.beginGroup("Frame");
    data.frameSync = loaded_settings.value("FrameSync").toString().trimmed();
    if (data.frameSync.isEmpty() || !hex_pattern.match(data.frameSync).hasMatch())
    {
        emit logMessage("  WARNING: Invalid FrameSync '" + data.frameSync +
                        "', using default " + PCMConstants::kDefaultFrameSync);
        data.frameSync = PCMConstants::kDefaultFrameSync;
    }
    loaded_settings.endGroup();

    // --- Parameters ---
    loaded_settings.beginGroup("Parameters");
    data.negativePolarity = loaded_settings.value("NegativePolarity").toBool();

    bool slope_ok;
    data.slopeIndex = loaded_settings.value("Slope").toInt(&slope_ok);
    if (!slope_ok || data.slopeIndex < 0 || data.slopeIndex > UIConstants::kMaxSlopeIndex)
    {
        emit logMessage("  WARNING: Invalid Slope " + loaded_settings.value("Slope").toString() +
                        ", using default " + QString::number(UIConstants::kDefaultSlopeIndex));
        data.slopeIndex = UIConstants::kDefaultSlopeIndex;
    }

    bool scale_ok;
    data.scale = loaded_settings.value("Scale").toString().trimmed();
    double scale_value = data.scale.toDouble(&scale_ok);
    if (!scale_ok || scale_value <= 0)
    {
        emit logMessage("  WARNING: Invalid Scale '" + data.scale +
                        "', using default " + UIConstants::kDefaultScale);
        data.scale = UIConstants::kDefaultScale;
    }
    loaded_settings.endGroup();

    // --- Receivers ---
    loaded_settings.beginGroup("Receivers");
    bool count_ok, channels_ok;
    data.receiverCount = loaded_settings.value("Count").toInt(&count_ok);
    if (!count_ok || data.receiverCount < UIConstants::kMinReceiverCount ||
        data.receiverCount > UIConstants::kMaxReceiverCount)
    {
        emit logMessage("  WARNING: Invalid receiver Count " +
                        loaded_settings.value("Count").toString() +
                        " (valid: " + QString::number(UIConstants::kMinReceiverCount) +
                        "-" + QString::number(UIConstants::kMaxReceiverCount) +
                        "), using default " + QString::number(UIConstants::kDefaultReceiverCount));
        data.receiverCount = UIConstants::kDefaultReceiverCount;
    }

    data.channelsPerReceiver = loaded_settings.value("ChannelsPerReceiver").toInt(&channels_ok);
    if (!channels_ok || data.channelsPerReceiver < UIConstants::kMinChannelsPerReceiver ||
        data.channelsPerReceiver > UIConstants::kMaxChannelsPerReceiver)
    {
        emit logMessage("  WARNING: Invalid ChannelsPerReceiver " +
                        loaded_settings.value("ChannelsPerReceiver").toString() +
                        " (valid: " + QString::number(UIConstants::kMinChannelsPerReceiver) +
                        "-" + QString::number(UIConstants::kMaxChannelsPerReceiver) +
                        "), using default " + QString::number(UIConstants::kDefaultChannelsPerReceiver));
        data.channelsPerReceiver = UIConstants::kDefaultChannelsPerReceiver;
    }

    int total_params = data.receiverCount * data.channelsPerReceiver;
    if (total_params > UIConstants::kMaxTotalParameters)
    {
        emit logMessage("  WARNING: Receivers x Channels (" + QString::number(total_params) +
                        ") exceeds maximum " + QString::number(UIConstants::kMaxTotalParameters) +
                        " words, using defaults");
        data.receiverCount = UIConstants::kDefaultReceiverCount;
        data.channelsPerReceiver = UIConstants::kDefaultChannelsPerReceiver;
    }
    loaded_settings.endGroup();

    // --- Time ---
    loaded_settings.beginGroup("Time");
    data.extractAllTime = loaded_settings.value("ExtractAllTime").toBool();
    bool rate_ok;
    data.sampleRateIndex = loaded_settings.value("SampleRate").toInt(&rate_ok);
    if (!rate_ok || data.sampleRateIndex < 0 || data.sampleRateIndex > UIConstants::kMaxSampleRateIndex)
    {
        emit logMessage("  WARNING: Invalid SampleRate " + loaded_settings.value("SampleRate").toString() +
                        ", using default 0");
        data.sampleRateIndex = 0;
    }
    loaded_settings.endGroup();

    emit logMessage("  FrameSync=" + data.frameSync +
                    ", Polarity=" + (data.negativePolarity ? "Negative" : "Positive") +
                    ", Slope=" + QString(UIConstants::kSlopeLabels[data.slopeIndex]) +
                    ", Scale=" + data.scale + " dB/V");
    emit logMessage("  Receivers=" + QString::number(data.receiverCount) +
                    ", Channels=" + QString::number(data.channelsPerReceiver) +
                    ", SampleRate=" + QString(UIConstants::kSampleRateLabels[data.sampleRateIndex]));
    emit logMessage("  Total parameters=" + QString::number(data.receiverCount * data.channelsPerReceiver) +
                    ", Frame=" + QString::number(data.receiverCount * data.channelsPerReceiver * PCMConstants::kCommonWordLen +
                                                  data.frameSync.length() * 4) + " bits");

    m_view_model->applySettingsData(data);
    m_view_model->loadFrameSetupFrom(filename);

    int param_count = m_view_model->frameSetup()->length();
    if (param_count > 0)
        emit logMessage("  Frame setup loaded: " + QString::number(param_count) + " parameters");
    else
        emit logMessage("  WARNING: No frame parameters found in file");
}

void SettingsManager::saveFile(const QString& filename)
{
    QSettings saved_settings(filename, QSettings::IniFormat);
    saved_settings.clear();

    SettingsData data = m_view_model->getSettingsData();

    saved_settings.beginGroup("Channels");
    saved_settings.setValue("TimeChannel", data.timeChannelId);
    saved_settings.setValue("PCMChannel", data.pcmChannelId);
    saved_settings.endGroup();

    saved_settings.beginGroup("Frame");
    saved_settings.setValue("FrameSync", data.frameSync);
    saved_settings.endGroup();

    saved_settings.beginGroup("Parameters");
    saved_settings.setValue("NegativePolarity", data.negativePolarity);
    saved_settings.setValue("Slope", data.slopeIndex);
    saved_settings.setValue("Scale", data.scale);
    saved_settings.endGroup();

    saved_settings.beginGroup("Time");
    saved_settings.setValue("ExtractAllTime", data.extractAllTime);
    saved_settings.setValue("SampleRate", data.sampleRateIndex);
    saved_settings.endGroup();

    saved_settings.beginGroup("Receivers");
    saved_settings.setValue("Count", data.receiverCount);
    saved_settings.setValue("ChannelsPerReceiver", data.channelsPerReceiver);
    saved_settings.endGroup();

    m_view_model->saveFrameSetupTo(saved_settings);
}
