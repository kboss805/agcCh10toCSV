#ifndef TST_CONSTANTS_H
#define TST_CONSTANTS_H

#include <QObject>

class TestConstants : public QObject
{
    Q_OBJECT

private slots:
    void pcmWordsInMinorFrame();
    void pcmBitsInMinorFrame();
    void pcmMaxChannelCount();
    void pcmDefaultFrameSync();
    void pcmCommonWordLen();
    void pcmNumMinorFrames();
    void pcmTimeRoundingOffset();
    void pcmChannelTypeIdentifiers();
    void uiDefaultReceiverCount();
    void uiDefaultChannelsPerReceiver();
    void uiDefaultScaleIndex();
    void uiDefaultRange();
    void uiTimeValidationLimits();
    void uiSampleRates();
    void uiChannelPrefixes();
    void uiNumKnownPrefixes();
    void uiOutputFormatConstants();
    void uiButtonText();
};

#endif // TST_CONSTANTS_H
