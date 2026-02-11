#include "tst_mainviewmodel_helpers.h"

#include <QRegularExpression>
#include <QtTest>

#include "mainviewmodel.h"

void TestMainViewModelHelpers::channelPrefixKnownIndices()
{
    MainViewModel vm;
    QCOMPARE(vm.channelPrefix(0), QString("L"));
    QCOMPARE(vm.channelPrefix(1), QString("R"));
    QCOMPARE(vm.channelPrefix(2), QString("C"));
}

void TestMainViewModelHelpers::channelPrefixUnknownIndices()
{
    MainViewModel vm;
    QCOMPARE(vm.channelPrefix(3), QString("CH4"));
    QCOMPARE(vm.channelPrefix(10), QString("CH11"));
    QCOMPARE(vm.channelPrefix(99), QString("CH100"));
}

void TestMainViewModelHelpers::parameterNameKnownChannels()
{
    MainViewModel vm;
    QCOMPARE(vm.parameterName(0, 0), QString("L_RCVR1"));
    QCOMPARE(vm.parameterName(1, 2), QString("R_RCVR3"));
    QCOMPARE(vm.parameterName(2, 15), QString("C_RCVR16"));
}

void TestMainViewModelHelpers::parameterNameUnknownChannels()
{
    MainViewModel vm;
    QCOMPARE(vm.parameterName(3, 0), QString("CH4_RCVR1"));
    QCOMPARE(vm.parameterName(5, 4), QString("CH6_RCVR5"));
}

void TestMainViewModelHelpers::generateOutputFilenameFormat()
{
    MainViewModel vm;
    QString filename = vm.generateOutputFilename();

    QRegularExpression re("^output\\d{12}\\.csv$");
    QVERIFY2(re.match(filename).hasMatch(),
             qPrintable("Filename '" + filename + "' does not match expected pattern"));
}

void TestMainViewModelHelpers::generateOutputFilenameNonEmpty()
{
    MainViewModel vm;
    QString f1 = vm.generateOutputFilename();
    QString f2 = vm.generateOutputFilename();
    QVERIFY(!f1.isEmpty());
    QVERIFY(!f2.isEmpty());
    QVERIFY(f1.startsWith("output"));
    QVERIFY(f1.endsWith(".csv"));
    QVERIFY(f2.startsWith("output"));
    QVERIFY(f2.endsWith(".csv"));
}
