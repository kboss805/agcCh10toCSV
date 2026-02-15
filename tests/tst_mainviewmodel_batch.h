#ifndef TST_MAINVIEWMODEL_BATCH_H
#define TST_MAINVIEWMODEL_BATCH_H

#include <QObject>

class TestMainViewModelBatch : public QObject
{
    Q_OBJECT

private slots:
    void batchModeDefaultFalse();
    void batchFileCountDefaultZero();
    void batchFilesDefaultEmpty();
    void generateBatchOutputFilenameFormat();
    void generateBatchOutputFilenameSpecialChars();
    void batchStatusSummaryEmpty();
    void clearStateResetsBatchMode();
    void clearStateClearsBatchFiles();
    void cancelProcessingSetsBatchCancelled();
    void setBatchFilePcmChannelOutOfBoundsNoSignal();
    void setBatchFileTimeChannelOutOfBoundsNoSignal();
    void setBatchFileChannelNegativeIndexNoSignal();
};

#endif // TST_MAINVIEWMODEL_BATCH_H
