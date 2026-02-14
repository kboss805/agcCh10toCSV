#ifndef TST_CONSTANTS_H
#define TST_CONSTANTS_H

#include <QObject>

class TestConstants : public QObject
{
    Q_OBJECT

private slots:
    void pcmMaxChannelCount();
    void pcmDefaultFrameSync();
    void pcmCommonWordLen();
    void pcmNumMinorFrames();
    void pcmTimeRoundingOffset();
    void pcmChannelTypeIdentifiers();
    void uiDefaultReceiverCount();
    void uiDefaultChannelsPerReceiver();
    void uiDefaultSlopeIndex();
    void uiDefaultScale();
    void uiTimeValidationLimits();
    void uiSampleRates();
    void uiMaxSampleRateIndex();
    void uiSlopeLabels();
    void uiSampleRateLabels();
    void uiChannelPrefixes();
    void uiNumKnownPrefixes();
    void uiOutputFormatConstants();
    void uiButtonText();

    // v2.0 additions
    void appVersionValues();
    void appVersionToString();
    void pcmMaxRawSampleValue();
    void pcmDefaultBufferSize();
    void pcmProgressReportInterval();
    void uiQSettingsKeys();
    void uiThemeIdentifiers();
    void uiLayoutConstants();
    void uiTimeConversionConstants();
    void uiPolarityConstants();

    // v2.2 additions
    void uiRecentFilesConstants();
};

#endif // TST_CONSTANTS_H
