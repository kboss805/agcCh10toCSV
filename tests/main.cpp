#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QtTest>

#include "tst_channeldata.h"
#include "tst_chapter10reader.h"
#include "tst_constants.h"
#include "tst_frameprocessor.h"
#include "tst_framesetup.h"
#include "tst_mainviewmodel_batch.h"
#include "tst_mainviewmodel_helpers.h"
#include "tst_mainviewmodel_state.h"
#include "tst_plotviewmodel.h"
#include "tst_receivergridwidget.h"
#include "tst_settingsdialog.h"
#include "tst_settingsmanager.h"
#include "tst_timeextractionwidget.h"

/// Runs a single test suite and appends results to the shared log file.
template<typename T>
int runSuite(const QString& log_path)
{
    // Per-suite temp file for Qt Test output
    QString tmp = log_path + ".tmp";
    QStringList args = { "tests", "-o", tmp + ",txt" };
    QVector<QByteArray> arg_bytes;
    QVector<char*> arg_ptrs;
    for (const QString& a : args)
    {
        arg_bytes.append(a.toLocal8Bit());
        arg_ptrs.append(arg_bytes.last().data());
    }
    int suite_argc = static_cast<int>(arg_ptrs.size());

    T test;
    int result = QTest::qExec(&test, suite_argc, arg_ptrs.data());

    // Append per-suite output to the combined log
    QFile src(tmp);
    if (src.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QFile dst(log_path);
        if (!dst.open(QIODevice::Append | QIODevice::Text))
        {
            return result;
        }
        dst.write(src.readAll());
        src.close();
        src.remove();
    }

    return result;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Use separate QSettings scope so tests don't overwrite real app settings
    app.setOrganizationName("agcCh10toCSV_tests");
    app.setApplicationName("agcCh10toCSV_tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Determine output log path: use -o argument if provided, else output/results.txt
    // output/ is gitignored so results never clutter the repo.
    QString log_path = "output/results.txt";
    for (int i = 1; i < argc; ++i)
    {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "-o" && i + 1 < argc)
        {
            // Strip ",txt" or ",tap" suffix if present
            log_path = QString::fromLocal8Bit(argv[i + 1]).split(',').first();
            break;
        }
    }

    // Clear the log file
    QFile(log_path).remove();

    int status = 0;

    status |= runSuite<TestChannelData>(log_path);
    status |= runSuite<TestChapter10Reader>(log_path);
    status |= runSuite<TestConstants>(log_path);
    status |= runSuite<TestFrameProcessor>(log_path);
    status |= runSuite<TestMainViewModelHelpers>(log_path);
    status |= runSuite<TestMainViewModelState>(log_path);
    status |= runSuite<TestFrameSetup>(log_path);
    status |= runSuite<TestSettingsDialog>(log_path);
    status |= runSuite<TestSettingsManager>(log_path);
    status |= runSuite<TestMainViewModelBatch>(log_path);
    status |= runSuite<TestPlotViewModel>(log_path);
    status |= runSuite<TestTimeExtractionWidget>(log_path);
    status |= runSuite<TestReceiverGridWidget>(log_path);

    return status;
}
