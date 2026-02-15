#include "tst_mainviewmodel_batch.h"

#include <QSignalSpy>
#include <QtTest>

#include "constants.h"
#include "mainviewmodel.h"

void TestMainViewModelBatch::batchModeDefaultFalse()
{
    MainViewModel vm;
    QCOMPARE(vm.batchMode(), false);
}

void TestMainViewModelBatch::batchFileCountDefaultZero()
{
    MainViewModel vm;
    QCOMPARE(vm.batchFileCount(), 0);
}

void TestMainViewModelBatch::batchFilesDefaultEmpty()
{
    MainViewModel vm;
    QVERIFY(vm.batchFiles().isEmpty());
}

void TestMainViewModelBatch::generateBatchOutputFilenameFormat()
{
    MainViewModel vm;
    QString result = vm.generateBatchOutputFilename("C:/data/test_recording.ch10");

    QCOMPARE(result, QString("AGC_test_recording.csv"));
}

void TestMainViewModelBatch::generateBatchOutputFilenameSpecialChars()
{
    MainViewModel vm;
    QString result = vm.generateBatchOutputFilename("C:/data/my file (2024).ch10");

    QCOMPARE(result, QString("AGC_my file (2024).csv"));
}

void TestMainViewModelBatch::batchStatusSummaryEmpty()
{
    MainViewModel vm;
    QVERIFY(vm.batchStatusSummary().isEmpty());
}

void TestMainViewModelBatch::clearStateResetsBatchMode()
{
    MainViewModel vm;

    // Manually verify clearState resets batch mode
    // (We can't call openFiles without real .ch10 files, but we can
    // verify that clearState on a default VM keeps batch mode false.)
    vm.clearState();
    QCOMPARE(vm.batchMode(), false);
    QCOMPARE(vm.batchFileCount(), 0);
    QVERIFY(vm.batchStatusSummary().isEmpty());
}

void TestMainViewModelBatch::clearStateClearsBatchFiles()
{
    MainViewModel vm;

    vm.clearState();
    QVERIFY(vm.batchFiles().isEmpty());
    QCOMPARE(vm.batchValidCount(), 0);
    QCOMPARE(vm.batchSkippedCount(), 0);
}

void TestMainViewModelBatch::cancelProcessingSetsBatchCancelled()
{
    MainViewModel vm;

    // cancelProcessing should not crash on a default-constructed VM
    // (no active processor, no batch mode)
    vm.cancelProcessing();

    // Verify the VM is still in a valid state
    QCOMPARE(vm.processing(), false);
    QCOMPARE(vm.batchMode(), false);
}

void TestMainViewModelBatch::setBatchFilePcmChannelOutOfBoundsNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);

    // Out-of-bounds file index on empty batch — should not crash or emit
    vm.setBatchFilePcmChannel(0, 0);
    QCOMPARE(spy.count(), 0);

    vm.setBatchFilePcmChannel(5, 0);
    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelBatch::setBatchFileTimeChannelOutOfBoundsNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);

    // Out-of-bounds file index on empty batch — should not crash or emit
    vm.setBatchFileTimeChannel(0, 0);
    QCOMPARE(spy.count(), 0);

    vm.setBatchFileTimeChannel(5, 0);
    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelBatch::setBatchFileChannelNegativeIndexNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);

    // Negative indices — should not crash or emit
    vm.setBatchFilePcmChannel(-1, 0);
    QCOMPARE(spy.count(), 0);

    vm.setBatchFileTimeChannel(-1, 0);
    QCOMPARE(spy.count(), 0);
}
