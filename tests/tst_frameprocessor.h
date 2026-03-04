/**
 * @file tst_frameprocessor.h
 * @brief Unit tests for FrameProcessor — static helpers, pre-scan, and processing.
 */

#ifndef TST_FRAMEPROCESSOR_H
#define TST_FRAMEPROCESSOR_H

#include <QObject>

class TestFrameProcessor : public QObject
{
    Q_OBJECT

private slots:
    void constructorDefaults();
    void requestAbortSetsFlag();
    void hasSyncPatternFindsMatch();
    void hasSyncPatternNoMatch();
    void hasSyncPatternShortBuffer();
    void derandomizeShortBufferIdentity();
    void derandomizeLongerBufferChanges();
    void writeTimeSampleFormat();
    void writeTimeSampleAveraging();
    void preScanInvalidChannelId();
    void preScanInvalidFile();
    void preScanWithNrzlFile();
    void preScanWithRnrzlFile();
    void processInvalidTimeChannel();
    void processInvalidPcmChannel();
    void processInvalidFile();
    void processWithTestFile();
};

#endif // TST_FRAMEPROCESSOR_H
