#include "mainview.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss_file(":/resources/win11-dark.qss");
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
