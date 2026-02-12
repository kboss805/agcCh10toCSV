#include <QApplication>
#include <QtTest>

#include "tst_channeldata.h"
#include "tst_constants.h"
#include "tst_framesetup.h"
#include "tst_mainviewmodel_helpers.h"
#include "tst_mainviewmodel_state.h"
#include "tst_settingsdialog.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    int status = 0;

    {
        TestChannelData test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestConstants test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestMainViewModelHelpers test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestMainViewModelState test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestFrameSetup test;
        status |= QTest::qExec(&test, argc, argv);
    }
    {
        TestSettingsDialog test;
        status |= QTest::qExec(&test, argc, argv);
    }

    return status;
}
