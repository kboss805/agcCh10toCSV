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
    bool polarity_ok;
    data.polarityIndex = loaded_settings.value("Polarity").toInt(&polarity_ok);
    if (!polarity_ok || data.polarityIndex < 0 || data.polarityIndex > UIConstants::kMaxPolarityIndex)
    {
        emit logMessage("  WARNING: Invalid Polarity " + loaded_settings.value("Polarity").toString() +
                        ", using default " + QString::number(UIConstants::kDefaultPolarityIndex));
        data.polarityIndex = UIConstants::kDefaultPolarityIndex;
    }

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
                        ", using default " + QString::number(UIConstants::kDefaultSampleRateIndex));
        data.sampleRateIndex = UIConstants::kDefaultSampleRateIndex;
    }
    loaded_settings.endGroup();

    emit logMessage("  FrameSync=" + data.frameSync +
                    ", Polarity=" + QString(UIConstants::kPolarityLabels[data.polarityIndex]) +
                    ", Slope=" + QString(UIConstants::kSlopeLabels[data.slopeIndex]) +
                    ", Scale=" + data.scale + " dB/V");
    emit logMessage("  Receivers=" + QString::number(data.receiverCount) +
                    ", Channels=" + QString::number(data.channelsPerReceiver) +
                    ", SampleRate=" + QString(UIConstants::kSampleRateLabels[data.sampleRateIndex]));
    emit logMessage("  Total parameters=" + QString::number(data.receiverCount * data.channelsPerReceiver) +
                    ", Frame=" + QString::number(data.receiverCount * data.channelsPerReceiver * PCMConstants::kCommonWordLen +
                                                  data.frameSync.length() * 4) + " bits");

    // Count parameter sections in the INI file (groups with a "Word" key, excluding reserved groups)
    static const QStringList reserved_groups = {"Defaults", "Frame", "Parameters", "Time", "Receivers", "Bounds"};
    int ini_param_count = 0;
    for (const QString& group : loaded_settings.childGroups())
    {
        if (reserved_groups.contains(group))
            continue;
        loaded_settings.beginGroup(group);
        if (loaded_settings.contains("Word"))
            ini_param_count++;
        loaded_settings.endGroup();
    }

    int expected_params = data.receiverCount * data.channelsPerReceiver;
    if (ini_param_count != expected_params)
    {
        emit logMessage("  WARNING: INI file has " + QString::number(ini_param_count) +
                        " parameter sections but Receivers (" + QString::number(data.receiverCount) +
                        ") x Channels (" + QString::number(data.channelsPerReceiver) +
                        ") = " + QString::number(expected_params));
    }

    m_view_model->applySettingsData(data);
    emit logMessage("  Frame setup: " + QString::number(m_view_model->frameSetup()->length()) +
                    " parameters (from startup config)");
}

void SettingsManager::saveFile(const QString& filename)
{
    QSettings saved_settings(filename, QSettings::IniFormat);
    saved_settings.clear();

    SettingsData data = m_view_model->getSettingsData();

    saved_settings.beginGroup("Frame");
    saved_settings.setValue("FrameSync", data.frameSync);
    saved_settings.endGroup();

    saved_settings.beginGroup("Parameters");
    saved_settings.setValue("Polarity", data.polarityIndex);
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

    int param_count = m_view_model->frameSetup()->length();
    emit logMessage("Settings saved: " + QFileInfo(filename).fileName());
    emit logMessage("  FrameSync=" + data.frameSync +
                    ", Polarity=" + QString(UIConstants::kPolarityLabels[data.polarityIndex]) +
                    ", Slope=" + QString(UIConstants::kSlopeLabels[data.slopeIndex]) +
                    ", Scale=" + data.scale + " dB/V");
    emit logMessage("  Receivers=" + QString::number(data.receiverCount) +
                    ", Channels=" + QString::number(data.channelsPerReceiver) +
                    ", SampleRate=" + QString(UIConstants::kSampleRateLabels[data.sampleRateIndex]) +
                    ", Parameters=" + QString::number(param_count));
    emit logMessage("  These settings are active and will be used for the next process.");
}
