#include "tst_constants.h"

#include <QtTest>

#include "constants.h"

void TestConstants::pcmWordsInMinorFrame()
{
    QCOMPARE(PCMConstants::kWordsInMinorFrame, 49);
}

void TestConstants::pcmBitsInMinorFrame()
{
    QCOMPARE(PCMConstants::kBitsInMinorFrame, 800);
}

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

void TestConstants::uiDefaultScaleIndex()
{
    QCOMPARE(UIConstants::kDefaultScaleIndex, 2);
}

void TestConstants::uiDefaultRange()
{
    QCOMPARE(QString(UIConstants::kDefaultRange), QString("100"));
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
    QCOMPARE(UIConstants::kSampleRate20Hz, 20);
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
