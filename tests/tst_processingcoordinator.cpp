#include "tst_processingcoordinator.h"

#include <QSignalSpy>
#include <QVector>
#include <QtTest>

#include "batchfileinfo.h"
#include "chapter10reader.h"
#include "framesetup.h"
#include "processingcoordinator.h"
#include "processingparams.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Builds a minimal 1-parameter FrameSetup via a temp INI file.
static FrameSetup* makeFrameSetup(QObject* parent = nullptr)
{
    FrameSetup* fs = new FrameSetup(parent);
    // words_in_frame for 1 receiver x 1 channel = 2 (1 data + 1 sync)
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (tmp.open())
    {
        tmp.write("[L_RCVR1]\nWord=1\n");
        tmp.flush();
        tmp.close();
        fs->tryLoadingFile(tmp.fileName(), 2);
    }
    return fs;
}

/// Returns a QVector<QVector<bool>> with one receiver and one channel, all enabled.
static QVector<QVector<bool>> singleReceiverStates()
{
    QVector<QVector<bool>> states(1);
    states[0].fill(true, 1);
    return states;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void TestProcessingCoordinator::constructorDefaults()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup fs(nullptr);

    ProcessingCoordinator coord(&batch_files, &fs, &reader);

    QCOMPARE(coord.processing(), false);
    QCOMPARE(coord.progressPercent(), 0);
    QCOMPARE(coord.isRandomized(), false);
}

void TestProcessingCoordinator::resetClearsBatchState()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup fs(nullptr);

    ProcessingCoordinator coord(&batch_files, &fs, &reader);

    // reset() should be callable at any time and leave state clean
    coord.reset();

    QCOMPARE(coord.processing(), false);
    QCOMPARE(coord.progressPercent(), 0);
    QCOMPARE(coord.isRandomized(), false);
}

void TestProcessingCoordinator::cancelProcessingNoRunNoOp()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup fs(nullptr);

    ProcessingCoordinator coord(&batch_files, &fs, &reader);

    // Should not crash when no thread is running
    coord.cancelProcessing();

    QCOMPARE(coord.processing(), false);
}

void TestProcessingCoordinator::runPreScanInvalidChannelReturnsFalse()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup* fs = makeFrameSetup();

    ProcessingCoordinator coord(&batch_files, fs, &reader);

    bool result = coord.runPreScan(-1, "any_file.ch10", "FE6B2840");

    QCOMPARE(result, false);

    delete fs;
}

void TestProcessingCoordinator::runPreScanInvalidFileReturnsFalse()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup* fs = makeFrameSetup();

    ProcessingCoordinator coord(&batch_files, fs, &reader);

    // Channel ID 0 is valid but the file does not exist → preScan fails
    bool result = coord.runPreScan(0, "nonexistent_file_xyz.ch10", "FE6B2840");

    QCOMPARE(result, false);

    delete fs;
}

void TestProcessingCoordinator::startSingleProcessingNoReceiversReturnsFalse()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup* fs = makeFrameSetup();

    ProcessingCoordinator coord(&batch_files, fs, &reader);

    QSignalSpy error_spy(&coord, &ProcessingCoordinator::errorOccurred);

    ProcessingParams params;
    params.outfile = "test_out.csv";

    // All receivers disabled — should return false and emit errorOccurred
    QVector<QVector<bool>> no_receivers(1);
    no_receivers[0].fill(false, 1);

    bool started = coord.startSingleProcessing(params, no_receivers);

    QCOMPARE(started, false);
    QVERIFY(!error_spy.isEmpty());
    QCOMPARE(coord.processing(), false);

    delete fs;
}

void TestProcessingCoordinator::startSingleProcessingEmitsProcessingState()
{
    QVector<BatchFileInfo> batch_files;
    Chapter10Reader reader;
    FrameSetup* fs = makeFrameSetup();

    ProcessingCoordinator coord(&batch_files, fs, &reader);

    QSignalSpy state_spy(&coord, &ProcessingCoordinator::processingStateChanged);

    ProcessingParams params;
    params.filename    = "nonexistent_but_coords_just_launch_thread.ch10";
    params.outfile     = "test_out.csv";
    params.calibration.scale_lower_bound = -100.0;
    params.calibration.scale_upper_bound = 0.0;
    params.calibration.negative_polarity = false;

    bool started = coord.startSingleProcessing(params, singleReceiverStates());

    // startSingleProcessing with at least one receiver enabled should emit
    // processingStateChanged(true) and return true even if the file doesn't exist
    // (the worker thread will fail, but the signal fires synchronously before launch)
    QCOMPARE(started, true);
    QVERIFY(!state_spy.isEmpty());
    QCOMPARE(state_spy.first().first().toBool(), true);

    // Clean up: the worker thread will fail quickly on the bad file; wait for it
    coord.cancelProcessing();
    QTest::qWait(200);

    delete fs;
}

void TestProcessingCoordinator::batchFilesUpdatedAfterPreScan()
{
    // Files with empty pcmChannelIds short-circuit the pre-scan bounds check
    // without requiring filesystem access.
    QVector<BatchFileInfo> batch_files;
    BatchFileInfo info;
    info.filepath = "nonexistent.ch10";
    info.filename = "nonexistent.ch10";
    info.skip     = false;
    // resolvedPcmIndex=-1 → bounds check fails → preScanOk stays false → batchFilesUpdated emitted
    info.resolvedPcmIndex = -1;
    batch_files.append(info);

    Chapter10Reader reader;
    FrameSetup* fs = makeFrameSetup();

    ProcessingCoordinator coord(&batch_files, fs, &reader);
    QSignalSpy updated_spy(&coord, &ProcessingCoordinator::batchFilesUpdated);

    coord.startBatchProcessing("output_dir", 0,
                               "FE6B2840", 0, 0, "1.0",
                               singleReceiverStates());

    // batchFilesUpdated is emitted once by preScanBatchFiles (synchronously),
    // and again when the batch state machine completes with no files to process.
    QVERIFY(updated_spy.count() >= 1);

    QTest::qWait(100);

    delete fs;
}
