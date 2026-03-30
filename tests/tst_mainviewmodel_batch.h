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
    void generateBatchOutputFilenamePath();
    void batchStatusSummaryEmpty();
    void clearStateResetsBatchMode();
    void clearStateClearsBatchFiles();
    void cancelProcessingSetsBatchCancelled();
    void setBatchFilePcmChannelOutOfBoundsNoSignal();
    void setBatchFileTimeChannelOutOfBoundsNoSignal();
    void setBatchFileChannelNegativeIndexNoSignal();

    // reorderBatchFile guard conditions
    void reorderBatchFileEmptyBatchNoSignal();
    void reorderBatchFileOutOfBoundsNoSignal();
    void reorderBatchFileSameIndexNoSignal();

    // retryFailedFiles guard conditions
    void retryFailedFilesNotBatchModeNoOp();

    // Channel setter skip-flag correctness
    void setBatchFilePcmChannelNoSkipWhenTimeResolved();
    void setBatchFilePcmChannelSkipWhenTimeNotResolved();
    void setBatchFileTimeChannelNoSkipWhenPcmResolved();

    // Retry state correctness
    void retryFailedFilesPreservesSuccessfulFile();
    void retryFailedFilesEmitsBatchFilesChanged();
};

#endif // TST_MAINVIEWMODEL_BATCH_H
