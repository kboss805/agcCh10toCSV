#include "tst_settingsmanager.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

#include "constants.h"
#include "framesetup.h"
#include "mainviewmodel.h"

static QString writeTemporaryIni(const QByteArray& content)
{
    QTemporaryFile* tmp = new QTemporaryFile;
    tmp->setAutoRemove(true);
    if (!tmp->open())
        return {};
    tmp->write(content);
    tmp->flush();
    tmp->close();
    // Keep file alive by leaking — auto-removed on process exit
    return tmp->fileName();
}

void TestSettingsManager::loadFileValidIni()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=2\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n\n"
        "[L_RCVR2]\nWord=2\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.frameSync(), QString("FE6B2840"));
    QCOMPARE(vm.polarityIndex(), 1);
    QCOMPARE(vm.slopeIndex(), 2);
    QCOMPARE(vm.scale(), QString("100"));
    QCOMPARE(vm.receiverCount(), 2);
    QCOMPARE(vm.channelsPerReceiver(), 1);
    QCOMPARE(vm.extractAllTime(), true);
    QCOMPARE(vm.sampleRateIndex(), 0);

    // No WARNING messages expected for valid INI
    bool has_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("WARNING"))
            has_warning = true;
    QVERIFY(!has_warning);
}

void TestSettingsManager::loadFileInvalidFrameSync()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=ZZZZ\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.frameSync(), QString(PCMConstants::kDefaultFrameSync));

    bool has_sync_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid FrameSync"))
            has_sync_warning = true;
    QVERIFY(has_sync_warning);
}

void TestSettingsManager::loadFileInvalidSlope()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=99\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.slopeIndex(), UIConstants::kDefaultSlopeIndex);

    bool has_slope_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid Slope"))
            has_slope_warning = true;
    QVERIFY(has_slope_warning);
}

void TestSettingsManager::loadFileInvalidScale()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=-5\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.scale(), QString(UIConstants::kDefaultScale));

    bool has_scale_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid Scale"))
            has_scale_warning = true;
    QVERIFY(has_scale_warning);
}

void TestSettingsManager::loadFileInvalidReceiverCount()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=0\nChannelsPerReceiver=3\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.receiverCount(), UIConstants::kDefaultReceiverCount);

    bool has_count_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid receiver Count"))
            has_count_warning = true;
    QVERIFY(has_count_warning);
}

void TestSettingsManager::loadFileInvalidChannelsPerReceiver()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=0\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    QCOMPARE(vm.channelsPerReceiver(), UIConstants::kDefaultChannelsPerReceiver);

    bool has_channels_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid ChannelsPerReceiver"))
            has_channels_warning = true;
    QVERIFY(has_channels_warning);
}

void TestSettingsManager::loadFilePolarityNegative()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    vm.loadSettings(path);

    QCOMPARE(vm.polarityIndex(), 1);
}

void TestSettingsManager::loadFilePolarityPositive()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=0\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    vm.loadSettings(path);

    QCOMPARE(vm.polarityIndex(), 0);
}

void TestSettingsManager::loadFilePolarityInvalid()
{
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=99\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=1\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    // Invalid polarity defaults to kDefaultPolarityIndex (1 = Negative)
    QCOMPARE(vm.polarityIndex(), UIConstants::kDefaultPolarityIndex);

    bool has_polarity_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("Invalid Polarity"))
            has_polarity_warning = true;
    QVERIFY(has_polarity_warning);
}

void TestSettingsManager::loadFileExceedsTotalParameters()
{
    // Count=16 x ChannelsPerReceiver=4 = 64, exceeds kMaxTotalParameters (48)
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=16\nChannelsPerReceiver=4\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    // Should fall back to defaults
    QCOMPARE(vm.receiverCount(), UIConstants::kDefaultReceiverCount);
    QCOMPARE(vm.channelsPerReceiver(), UIConstants::kDefaultChannelsPerReceiver);

    bool has_exceeds_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("exceeds maximum"))
            has_exceeds_warning = true;
    QVERIFY(has_exceeds_warning);
}

void TestSettingsManager::saveFileRoundtrip()
{
    // Create a valid INI, load it, save to a new file, load again, compare
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=DEADBEEF\n\n"
        "[Parameters]\nPolarity=0\nSlope=1\nScale=50\n\n"
        "[Time]\nExtractAllTime=false\nSampleRate=2\n\n"
        "[Receivers]\nCount=4\nChannelsPerReceiver=2\n\n"
        "[L_RCVR1]\nWord=1\n\n[R_RCVR1]\nWord=2\n\n"
        "[L_RCVR2]\nWord=3\n\n[R_RCVR2]\nWord=4\n\n"
        "[L_RCVR3]\nWord=5\n\n[R_RCVR3]\nWord=6\n\n"
        "[L_RCVR4]\nWord=7\n\n[R_RCVR4]\nWord=8\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    vm.loadSettings(path);

    // Save to a new temp file — use a plain path so QSettings can write freely
    QString save_path = QDir::tempPath() + "/agcCh10toCSV_test_roundtrip.ini";
    vm.saveSettings(save_path);

    // Verify the saved INI file contains expected values
    QSettings verify(save_path, QSettings::IniFormat);
    QCOMPARE(verify.value("Frame/FrameSync").toString(), QString("DEADBEEF"));
    QCOMPARE(verify.value("Parameters/Polarity").toInt(), 0);
    QCOMPARE(verify.value("Parameters/Slope").toInt(), 1);
    QCOMPARE(verify.value("Parameters/Scale").toString(), QString("50"));
    QCOMPARE(verify.value("Time/ExtractAllTime").toBool(), false);
    QCOMPARE(verify.value("Time/SampleRate").toInt(), 2);
    QCOMPARE(verify.value("Receivers/Count").toInt(), 4);
    QCOMPARE(verify.value("Receivers/ChannelsPerReceiver").toInt(), 2);

    // Load the saved file into a new ViewModel
    MainViewModel vm2;
    vm2.loadSettings(save_path);

    QCOMPARE(vm2.frameSync(), QString("DEADBEEF"));
    QCOMPARE(vm2.polarityIndex(), 0);
    QCOMPARE(vm2.slopeIndex(), 1);
    QCOMPARE(vm2.scale(), QString("50"));
    QCOMPARE(vm2.extractAllTime(), false);
    QCOMPARE(vm2.sampleRateIndex(), 2);
    QCOMPARE(vm2.receiverCount(), 4);
    QCOMPARE(vm2.channelsPerReceiver(), 2);

    QFile::remove(save_path);
}

void TestSettingsManager::loadFileParameterCountMismatch()
{
    // Count=2 x ChannelsPerReceiver=1 = 2, but INI has 3 parameter sections
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=2\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n\n"
        "[L_RCVR2]\nWord=2\n\n"
        "[L_RCVR3]\nWord=3\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::logMessageReceived);
    vm.loadSettings(path);

    bool has_mismatch_warning = false;
    for (const auto& call : spy)
        if (call.at(0).toString().contains("parameter sections"))
            has_mismatch_warning = true;
    QVERIFY(has_mismatch_warning);
}

void TestSettingsManager::loadFilePreservesFrameSetup()
{
    // Loading an INI with different receiver count should NOT change frame setup.
    // Frame setup stays as loaded from default.ini at construction.
    QString path = writeTemporaryIni(
        "[Frame]\nFrameSync=FE6B2840\n\n"
        "[Parameters]\nPolarity=1\nSlope=2\nScale=100\n\n"
        "[Time]\nExtractAllTime=true\nSampleRate=0\n\n"
        "[Receivers]\nCount=2\nChannelsPerReceiver=1\n\n"
        "[L_RCVR1]\nWord=1\n\n"
        "[L_RCVR2]\nWord=2\n");
    QVERIFY(!path.isEmpty());

    MainViewModel vm;
    int original_length = vm.frameSetup()->length();
    vm.loadSettings(path);

    // Frame setup should NOT be reloaded from the settings INI
    QCOMPARE(vm.frameSetup()->length(), original_length);
    // But receiver count should be updated
    QCOMPARE(vm.receiverCount(), 2);
    QCOMPARE(vm.channelsPerReceiver(), 1);
}
