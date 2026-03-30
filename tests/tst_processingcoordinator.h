#ifndef TST_PROCESSINGCOORDINATOR_H
#define TST_PROCESSINGCOORDINATOR_H

#include <QObject>

class TestProcessingCoordinator : public QObject
{
    Q_OBJECT

private slots:
    void constructorDefaults();
    void resetClearsBatchState();
    void cancelProcessingNoRunNoOp();
    void runPreScanInvalidChannelReturnsFalse();
    void runPreScanInvalidFileReturnsFalse();
    void startSingleProcessingNoReceiversReturnsFalse();
    void startSingleProcessingEmitsProcessingState();
    void batchFilesUpdatedAfterPreScan();
};

#endif // TST_PROCESSINGCOORDINATOR_H
