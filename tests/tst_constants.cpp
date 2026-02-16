#include "tst_constants.h"

#include <QtTest>

#include "constants.h"

void TestConstants::pcmMaxChannelCount()
{
    QCOMPARE(PCMConstants::kMaxChannelCount, 0x10000);
}

void TestConstants::pcmDefaultFrameSync()
{
    QCOMPARE(QString(PCMConstants::kDefaultFrameSync), QString("FE6B2840"));
}

void TestConstants::pcmCommonWordLen()
{
    QCOMPARE(PCMConstants::kCommonWordLen, 16);
}

void TestConstants::pcmNumMinorFrames()
{
    QCOMPARE(PCMConstants::kNumMinorFrames, 1);
}

void TestConstants::pcmTimeRoundingOffset()
{
    QCOMPARE(PCMConstants::kTimeRoundingOffset, 0.0005);
}

void TestConstants::pcmChannelTypeIdentifiers()
{
    QCOMPARE(QString(PCMConstants::kChannelTypeTime), QString("TIMEIN"));
    QCOMPARE(QString(PCMConstants::kChannelTypePcm), QString("PCMIN"));
}

void TestConstants::uiDefaultReceiverCount()
{
    QCOMPARE(UIConstants::kDefaultReceiverCount, 16);
}

void TestConstants::uiDefaultChannelsPerReceiver()
{
    QCOMPARE(UIConstants::kDefaultChannelsPerReceiver, 3);
}

void TestConstants::uiDefaultSlopeIndex()
{
    QCOMPARE(UIConstants::kDefaultSlopeIndex, 2);
}

void TestConstants::uiDefaultScale()
{
    QCOMPARE(QString(UIConstants::kDefaultScale), QString("100"));
}

void TestConstants::uiTimeValidationLimits()
{
    QCOMPARE(UIConstants::kMinDayOfYear, 1);
    QCOMPARE(UIConstants::kMaxDayOfYear, 366);
    QCOMPARE(UIConstants::kMaxHour, 23);
    QCOMPARE(UIConstants::kMaxMinute, 59);
    QCOMPARE(UIConstants::kMaxSecond, 59);
}

void TestConstants::uiSampleRates()
{
    QCOMPARE(UIConstants::kSampleRate1Hz, 1);
    QCOMPARE(UIConstants::kSampleRate10Hz, 10);
    QCOMPARE(UIConstants::kSampleRate100Hz, 100);
}

void TestConstants::uiMaxSampleRateIndex()
{
    QCOMPARE(UIConstants::kMaxSampleRateIndex, 2);
}

void TestConstants::uiSlopeLabels()
{
    QCOMPARE(QString(UIConstants::kSlopeLabels[0]), QString("+/-10V"));
    QCOMPARE(QString(UIConstants::kSlopeLabels[1]), QString("+/-5V"));
    QCOMPARE(QString(UIConstants::kSlopeLabels[2]), QString("0-10V"));
    QCOMPARE(QString(UIConstants::kSlopeLabels[3]), QString("0-5V"));
}

void TestConstants::uiSampleRateLabels()
{
    QCOMPARE(QString(UIConstants::kSampleRateLabels[0]), QString("1 Hz"));
    QCOMPARE(QString(UIConstants::kSampleRateLabels[1]), QString("10 Hz"));
    QCOMPARE(QString(UIConstants::kSampleRateLabels[2]), QString("100 Hz"));
}

void TestConstants::uiChannelPrefixes()
{
    QCOMPARE(QString(UIConstants::kChannelPrefixes[0]), QString("L"));
    QCOMPARE(QString(UIConstants::kChannelPrefixes[1]), QString("R"));
    QCOMPARE(QString(UIConstants::kChannelPrefixes[2]), QString("C"));
}

void TestConstants::uiNumKnownPrefixes()
{
    QCOMPARE(UIConstants::kNumKnownPrefixes, 3);
}

void TestConstants::uiOutputFormatConstants()
{
    QCOMPARE(QString(UIConstants::kOutputTimestampFormat), QString("MMddyyhhmmss"));
    QCOMPARE(QString(UIConstants::kOutputPrefix), QString("output"));
    QCOMPARE(QString(UIConstants::kOutputExtension), QString(".csv"));
}

void TestConstants::uiButtonText()
{
    QCOMPARE(QString(UIConstants::kButtonTextStart), QString("Process"));
    QCOMPARE(QString(UIConstants::kButtonTextProcessing), QString("Processing..."));
}

// v2.0 additions

void TestConstants::appVersionValues()
{
    QCOMPARE(AppVersion::kMajor, 3);
    QCOMPARE(AppVersion::kMinor, 0);
    QCOMPARE(AppVersion::kPatch, 0);
}

void TestConstants::appVersionToString()
{
    QCOMPARE(AppVersion::toString(), QString("3.0.0"));
}

void TestConstants::pcmMaxRawSampleValue()
{
    QCOMPARE(PCMConstants::kMaxRawSampleValue, static_cast<uint16_t>(0xFFFF));
}

void TestConstants::pcmDefaultBufferSize()
{
    QCOMPARE(PCMConstants::kDefaultBufferSize, 65536UL);
}

void TestConstants::pcmProgressReportInterval()
{
    QCOMPARE(PCMConstants::kProgressReportInterval, 100);
}

void TestConstants::uiQSettingsKeys()
{
    QCOMPARE(QString(UIConstants::kOrganizationName), QString("agcCh10toCSV"));
    QCOMPARE(QString(UIConstants::kApplicationName), QString("agcCh10toCSV"));
    QCOMPARE(QString(UIConstants::kSettingsKeyTheme), QString("Theme"));
    QCOMPARE(QString(UIConstants::kSettingsKeyLastCh10Dir), QString("LastCh10Directory"));
    QCOMPARE(QString(UIConstants::kSettingsKeyLastCsvDir), QString("LastCsvDirectory"));
    QCOMPARE(QString(UIConstants::kSettingsKeyLastIniDir), QString("LastIniDirectory"));
}

void TestConstants::uiThemeIdentifiers()
{
    QCOMPARE(QString(UIConstants::kThemeDark), QString("dark"));
    QCOMPARE(QString(UIConstants::kThemeLight), QString("light"));
}

void TestConstants::uiLayoutConstants()
{
    QCOMPARE(UIConstants::kReceiverGridColumns, 4);
    QCOMPARE(UIConstants::kTreeItemHeightFactor, 24);
    QCOMPARE(UIConstants::kTreeHeightBuffer, 4);
    QCOMPARE(UIConstants::kTreeFixedWidth, 100);
    QCOMPARE(UIConstants::kLogMinimumWidth, 400);
}

void TestConstants::uiTimeConversionConstants()
{
    QCOMPARE(UIConstants::kSecondsPerDay, 86400);
    QCOMPARE(UIConstants::kSecondsPerHour, 3600);
    QCOMPARE(UIConstants::kSecondsPerMinute, 60);
}

void TestConstants::uiPolarityConstants()
{
    QCOMPARE(UIConstants::kDefaultPolarityIndex, 1);
    QCOMPARE(UIConstants::kMaxPolarityIndex, 1);
    QCOMPARE(QString(UIConstants::kPolarityLabels[0]), QString("Positive"));
    QCOMPARE(QString(UIConstants::kPolarityLabels[1]), QString("Negative"));
}

// v2.2 additions

void TestConstants::uiRecentFilesConstants()
{
    QCOMPARE(QString(UIConstants::kSettingsKeyRecentFiles), QString("RecentFiles"));
    QCOMPARE(UIConstants::kMaxRecentFiles, 5);
}

// v2.3 additions

void TestConstants::uiDeploymentConstants()
{
    QCOMPARE(QString(UIConstants::kPortableMarkerFilename), QString("portable"));
    QCOMPARE(QString(UIConstants::kSettingsDirName), QString("settings"));
    QCOMPARE(QString(UIConstants::kDefaultIniFilename), QString("default.ini"));
}

// v2.4 additions

void TestConstants::uiBatchConstants()
{
    QCOMPARE(QString(UIConstants::kBatchOutputPrefix), QString("AGC_"));
    QCOMPARE(QString(UIConstants::kSettingsKeyLastBatchDir), QString("LastBatchOutputDirectory"));
    QCOMPARE(UIConstants::kBatchFileListHeight, 180);
}

// v3.0 additions

void TestConstants::plotConstants()
{
    QCOMPARE(PlotConstants::kPlotDockMinWidth, 500);
    QCOMPARE(PlotConstants::kPlotDockMinHeight, 300);
    QCOMPARE(PlotConstants::kAxisMarginFactor, 0.05);
    QCOMPARE(QString(PlotConstants::kDefaultPlotTitle), QString("AGC Signal Plot"));
    QCOMPARE(QString(PlotConstants::kYAxisLabel), QString("Amplitude (dB)"));
    QCOMPARE(QString(PlotConstants::kXAxisLabel), QString("Time (s)"));
    QCOMPARE(PlotConstants::kZoomFactor, 0.1);
    QCOMPARE(PlotConstants::kNumReceiverColors, 10);
    QCOMPARE(QString(UIConstants::kSettingsKeyPlotVisible), QString("PlotVisible"));
}
