#include "tst_mainviewmodel_helpers.h"

#include <QRegularExpression>
#include <QtTest>

#include "mainviewmodel.h"

void TestMainViewModelHelpers::channelPrefixKnownIndices()
{
    QCOMPARE(MainViewModel::channelPrefix(0), QString("L"));
    QCOMPARE(MainViewModel::channelPrefix(1), QString("R"));
    QCOMPARE(MainViewModel::channelPrefix(2), QString("C"));
}

void TestMainViewModelHelpers::channelPrefixUnknownIndices()
{
    QCOMPARE(MainViewModel::channelPrefix(3), QString("CH4"));
    QCOMPARE(MainViewModel::channelPrefix(10), QString("CH11"));
    QCOMPARE(MainViewModel::channelPrefix(99), QString("CH100"));
}

void TestMainViewModelHelpers::parameterNameKnownChannels()
{
    QCOMPARE(MainViewModel::parameterName(0, 0), QString("L_RCVR1"));
    QCOMPARE(MainViewModel::parameterName(1, 2), QString("R_RCVR3"));
    QCOMPARE(MainViewModel::parameterName(2, 15), QString("C_RCVR16"));
}

void TestMainViewModelHelpers::parameterNameUnknownChannels()
{
    QCOMPARE(MainViewModel::parameterName(3, 0), QString("CH4_RCVR1"));
    QCOMPARE(MainViewModel::parameterName(5, 4), QString("CH6_RCVR5"));
}

void TestMainViewModelHelpers::generateOutputFilenameFormat()
{
    QString filename = MainViewModel::generateOutputFilename();

    QRegularExpression re("^output\\d{12}\\.csv$");
    QVERIFY2(re.match(filename).hasMatch(),
             qPrintable("Filename '" + filename + "' does not match expected pattern"));
}

void TestMainViewModelHelpers::generateOutputFilenameNonEmpty()
{
    QString f1 = MainViewModel::generateOutputFilename();
    QString f2 = MainViewModel::generateOutputFilename();
    QVERIFY(!f1.isEmpty());
    QVERIFY(!f2.isEmpty());
    QVERIFY(f1.startsWith("output"));
    QVERIFY(f1.endsWith(".csv"));
    QVERIFY(f2.startsWith("output"));
    QVERIFY(f2.endsWith(".csv"));
}
