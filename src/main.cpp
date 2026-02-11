#include "mainview.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QSettings>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSettings app_settings("agcCh10toCSV", "agcCh10toCSV");
    QString theme = app_settings.value("Theme", "dark").toString();
    QString qss_path = (theme == "light")
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
