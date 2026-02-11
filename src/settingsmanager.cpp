/**
 * @file settingsmanager.cpp
 * @brief Implementation of SettingsManager — INI file persistence.
 */

#include "settingsmanager.h"

#include <QSettings>

#include "constants.h"
#include "mainviewmodel.h"

SettingsManager::SettingsManager(MainViewModel* view_model) :
    m_view_model(view_model)
{

}

void SettingsManager::loadFile(const QString& filename)
{
    QSettings loaded_settings(filename, QSettings::IniFormat);

    SettingsData data;

    loaded_settings.beginGroup("Channels");
    data.timeChannelId = loaded_settings.value("TimeChannel").toInt();
    data.pcmChannelId = loaded_settings.value("PCMChannel").toInt();
    loaded_settings.endGroup();

    loaded_settings.beginGroup("Frame");
    data.frameSync = loaded_settings.value("FrameSync").toString();
    loaded_settings.endGroup();

    loaded_settings.beginGroup("Parameters");
    data.negativePolarity = loaded_settings.value("NegativePolarity").toBool();
    data.scaleIndex = loaded_settings.value("Scale").toInt();
    data.range = loaded_settings.value("Range").toString();
    loaded_settings.endGroup();

    loaded_settings.beginGroup("Receivers");
    data.receiverCount = loaded_settings.value("Count", UIConstants::kDefaultReceiverCount).toInt();
    data.channelsPerReceiver =
        loaded_settings.value("ChannelsPerReceiver", UIConstants::kDefaultChannelsPerReceiver).toInt();
    loaded_settings.endGroup();

    loaded_settings.beginGroup("Time");
    data.extractAllTime = loaded_settings.value("ExtractAllTime").toBool();
    data.sampleRateIndex = loaded_settings.value("SampleRate").toInt();
    loaded_settings.endGroup();

    m_view_model->applySettingsData(data);
    m_view_model->loadFrameSetupFrom(filename);
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
    saved_settings.setValue("Scale", data.scaleIndex);
    saved_settings.setValue("Range", data.range);
    saved_settings.endGroup();

    saved_settings.beginGroup("Receivers");
    saved_settings.setValue("Count", data.receiverCount);
    saved_settings.setValue("ChannelsPerReceiver", data.channelsPerReceiver);
    saved_settings.endGroup();

    saved_settings.beginGroup("Time");
    saved_settings.setValue("ExtractAllTime", data.extractAllTime);
    saved_settings.setValue("SampleRate", data.sampleRateIndex);
    saved_settings.endGroup();

    m_view_model->saveFrameSetupTo(saved_settings);
}
