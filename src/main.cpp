/**
 * @file main.cpp
 * @brief Application entry point — loads theme, creates MainView, and runs the event loop.
 */

#include "mainview.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QSettings>
#include <QStyleFactory>

#include "constants.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName(UIConstants::kOrganizationName);
    a.setApplicationName(UIConstants::kApplicationName);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings app_settings;
    QString theme = app_settings.value(UIConstants::kSettingsKeyTheme, UIConstants::kThemeDark).toString();
    QString qss_path = (theme == UIConstants::kThemeLight)
        ? ":/resources/win11-light.qss"
        : ":/resources/win11-dark.qss";

    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly))
    {
        QString styleSheet = QLatin1String(qss_file.readAll());
        qApp->setStyleSheet(styleSheet);
        qss_file.close();
    }

    a.setWindowIcon(QIcon(":/resources/icon.ico"));

    MainView w;
    w.show();
    return a.exec();
}
