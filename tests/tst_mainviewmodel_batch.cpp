#include "tst_mainviewmodel_batch.h"

#include <QSignalSpy>
#include <QtTest>

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

void TestMainViewModelBatch::generateBatchOutputFilenamePath()
{
    // Verify the full path assembled by the overwrite-check logic is correct
    QString out_dir = "C:/output";
    QString filepath = "C:/data/flight001.ch10";
    QString result = out_dir + "/" + MainViewModel::generateBatchOutputFilename(filepath);

    QCOMPARE(result, QString("C:/output/AGC_flight001.csv"));
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
    QCOMPARE(spy.size(), 0);

    vm.setBatchFilePcmChannel(5, 0);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::setBatchFileTimeChannelOutOfBoundsNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);

    // Out-of-bounds file index on empty batch — should not crash or emit
    vm.setBatchFileTimeChannel(0, 0);
    QCOMPARE(spy.size(), 0);

    vm.setBatchFileTimeChannel(5, 0);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::setBatchFileChannelNegativeIndexNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);

    // Negative indices — should not crash or emit
    vm.setBatchFilePcmChannel(-1, 0);
    QCOMPARE(spy.size(), 0);

    vm.setBatchFileTimeChannel(-1, 0);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::reorderBatchFileEmptyBatchNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFilesChanged);

    // Empty batch — any index is out-of-bounds; should not crash or emit
    vm.reorderBatchFile(0, 1);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::reorderBatchFileOutOfBoundsNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFilesChanged);

    vm.reorderBatchFile(-1, 0);
    QCOMPARE(spy.size(), 0);

    vm.reorderBatchFile(0, -1);
    QCOMPARE(spy.size(), 0);

    vm.reorderBatchFile(5, 0);
    QCOMPARE(spy.size(), 0);

    vm.reorderBatchFile(0, 5);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::reorderBatchFileSameIndexNoSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::batchFilesChanged);

    // from == to is a no-op regardless of whether files are loaded
    vm.reorderBatchFile(0, 0);
    QCOMPARE(spy.size(), 0);
}

void TestMainViewModelBatch::retryFailedFilesNotBatchModeNoOp()
{
    MainViewModel vm;

    // Not in batch mode — should not crash, should not start processing
    vm.retryFailedFiles();
    QCOMPARE(vm.processing(), false);
    QCOMPARE(vm.batchMode(), false);
}

// ---------------------------------------------------------------------------
// Channel setter skip-flag correctness
// ---------------------------------------------------------------------------

void TestMainViewModelBatch::setBatchFilePcmChannelNoSkipWhenTimeResolved()
{
    // If the time channel is already resolved, changing PCM must NOT mark the file skip.
    MainViewModel vm;
    BatchFileInfo info;
    info.pcmChannelStrings  = { "PCM-1" };
    info.timeChannelStrings = { "IRIG-B" };
    info.resolvedTimeIndex  = 0;   // time already resolved
    info.resolvedPcmIndex   = -1;
    vm.addBatchFileForTesting(info);

    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);
    vm.setBatchFilePcmChannel(0, 0);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(vm.batchFiles().first().resolvedPcmIndex, 0);
    QCOMPARE(vm.batchFiles().first().skip, false);
}

void TestMainViewModelBatch::setBatchFilePcmChannelSkipWhenTimeNotResolved()
{
    // If no time channel is resolved, changing PCM should mark the file skip.
    MainViewModel vm;
    BatchFileInfo info;
    info.pcmChannelStrings  = { "PCM-1" };
    info.timeChannelStrings = { "IRIG-B" };
    info.resolvedTimeIndex  = -1;  // time NOT resolved
    info.resolvedPcmIndex   = -1;
    vm.addBatchFileForTesting(info);

    vm.setBatchFilePcmChannel(0, 0);

    QCOMPARE(vm.batchFiles().first().skip, true);
}

void TestMainViewModelBatch::setBatchFileTimeChannelNoSkipWhenPcmResolved()
{
    // If the PCM channel is already resolved, changing Time must NOT mark the file skip.
    MainViewModel vm;
    BatchFileInfo info;
    info.pcmChannelStrings  = { "PCM-1" };
    info.timeChannelStrings = { "IRIG-B" };
    info.resolvedPcmIndex   = 0;   // PCM already resolved
    info.resolvedTimeIndex  = -1;
    vm.addBatchFileForTesting(info);

    QSignalSpy spy(&vm, &MainViewModel::batchFileUpdated);
    vm.setBatchFileTimeChannel(0, 0);

    QCOMPARE(spy.size(), 1);
    QCOMPARE(vm.batchFiles().first().resolvedTimeIndex, 0);
    QCOMPARE(vm.batchFiles().first().skip, false);
}

// ---------------------------------------------------------------------------
// Retry state correctness
// ---------------------------------------------------------------------------

void TestMainViewModelBatch::retryFailedFilesPreservesSuccessfulFile()
{
    // A file that already succeeded must not have processedOk or preScanOk cleared
    // by retryFailedFiles(), even when a failed sibling file is reset and re-tried.
    MainViewModel vm;

    BatchFileInfo success;
    success.pcmChannelStrings  = { "PCM-1" };
    success.timeChannelStrings = { "IRIG-B" };
    success.resolvedPcmIndex   = 0;
    success.resolvedTimeIndex  = 0;
    success.processed          = true;
    success.processedOk        = true;
    success.preScanOk          = true;
    vm.addBatchFileForTesting(success);

    // Failed file: pcmChannelIds intentionally empty so preScanBatchFiles()
    // exits its idx bounds check without touching the filesystem.
    BatchFileInfo failed;
    failed.pcmChannelStrings  = { "PCM-1" };
    failed.timeChannelStrings = { "IRIG-B" };
    failed.resolvedPcmIndex   = 0;
    failed.resolvedTimeIndex  = 0;
    failed.processed          = true;
    failed.processedOk        = false;
    failed.preScanOk          = false;
    vm.addBatchFileForTesting(failed);

    vm.retryFailedFiles();

    QCOMPARE(vm.batchFiles().at(0).processedOk, true);
    QCOMPARE(vm.batchFiles().at(0).processed,   true);
    QCOMPARE(vm.batchFiles().at(0).preScanOk,   true);
}

void TestMainViewModelBatch::retryFailedFilesEmitsBatchFilesChanged()
{
    // retryFailedFiles() must emit batchFilesChanged() at least once so the
    // view can rebuild the list after the retry batch completes.
    MainViewModel vm;

    BatchFileInfo failed;
    failed.pcmChannelStrings  = { "PCM-1" };
    failed.timeChannelStrings = { "IRIG-B" };
    failed.resolvedPcmIndex   = 0;
    failed.resolvedTimeIndex  = 0;
    failed.processed          = true;
    failed.processedOk        = false;
    failed.preScanOk          = false;
    vm.addBatchFileForTesting(failed);

    QSignalSpy spy(&vm, &MainViewModel::batchFilesChanged);
    vm.retryFailedFiles();

    QVERIFY(spy.size() >= 1);
}
