#include "framesetup.h"

namespace {
const QStringList kSettingsGroups = {
    "Defaults", "Channels", "Frame", "Parameters", "Time", "Receivers", "Bounds"
};
} // namespace

FrameSetup::FrameSetup(QObject* parent) :
    QObject(parent)
{

}

bool FrameSetup::tryLoadingFile(const QString& filename, int num_words_in_minor_frame)
{
    clearParameters();

    QSettings settings(filename, QSettings::IniFormat);
    if (settings.status() != QSettings::NoError)
        return false;

    QStringList groups = settings.childGroups();
    if (groups.isEmpty())
        return false;

    for (const QString& group : groups)
    {
        if (kSettingsGroups.contains(group))
            continue;

        settings.beginGroup(group);

        if (!settings.contains("Word"))
        {
            settings.endGroup();
            return false;
        }

        bool word_ok;
        int parameter_word = settings.value("Word").toInt(&word_ok) - 1;

        if (!word_ok || parameter_word < 0 || parameter_word >= num_words_in_minor_frame - 1)
        {
            settings.endGroup();
            return false;
        }

        ParameterInfo parameter = ParameterInfo();
        parameter.name = group;
        parameter.word = parameter_word;
        parameter.slope = 0;
        parameter.scale = 0;
        parameter.is_enabled = settings.value("Enabled", true).toBool();
        parameter.sample_sum = 0;

        m_parameters.append(parameter);

        settings.endGroup();
    }

    return true;
}

int FrameSetup::length() const
{
    return m_parameters.size();
}

const ParameterInfo* FrameSetup::getParameter(int i) const
{
    if (i < 0 || i >= m_parameters.size())
        return nullptr;
    return &(m_parameters[i]);
}

ParameterInfo* FrameSetup::getParameter(int i)
{
    if (i < 0 || i >= m_parameters.size())
        return nullptr;
    return &(m_parameters[i]);
}

void FrameSetup::clearParameters()
{
    m_parameters.clear();
}

void FrameSetup::saveToSettings(QSettings& settings)
{
    for (const auto& param : m_parameters)
    {
        settings.beginGroup(param.name);
        settings.setValue("Word", param.word + 1);
        settings.setValue("Enabled", param.is_enabled);
        settings.endGroup();
    }
}
