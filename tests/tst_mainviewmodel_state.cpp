#include "tst_mainviewmodel_state.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

#include "constants.h"
#include "framesetup.h"
#include "mainviewmodel.h"
#include "settingsdata.h"

void TestMainViewModelState::constructorDefaults()
{
    MainViewModel vm;

    QCOMPARE(vm.fileLoaded(), false);
    QCOMPARE(vm.processing(), false);
    QCOMPARE(vm.progressPercent(), 0);
    QCOMPARE(vm.extractAllTime(), true);
    QCOMPARE(vm.sampleRateIndex(), 0);
    QCOMPARE(vm.polarityIndex(), 0);
    QCOMPARE(vm.slopeIndex(), UIConstants::kDefaultSlopeIndex);
    QCOMPARE(vm.scale(), QString(UIConstants::kDefaultScale));
    QCOMPARE(vm.receiverCount(), UIConstants::kDefaultReceiverCount);
    QCOMPARE(vm.channelsPerReceiver(), UIConstants::kDefaultChannelsPerReceiver);
    QCOMPARE(vm.timeChannelIndex(), 0);
    QCOMPARE(vm.pcmChannelIndex(), 0);
    QVERIFY(vm.inputFilename().isEmpty());
}

void TestMainViewModelState::setExtractAllTimeEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::extractAllTimeChanged);

    vm.setExtractAllTime(false);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.extractAllTime(), false);
}

void TestMainViewModelState::setExtractAllTimeNoOpWhenUnchanged()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::extractAllTimeChanged);

    vm.setExtractAllTime(true); // default is true

    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelState::setSampleRateIndexEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::sampleRateIndexChanged);

    vm.setSampleRateIndex(2);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.sampleRateIndex(), 2);
}

void TestMainViewModelState::setSampleRateIndexNoOpWhenUnchanged()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::sampleRateIndexChanged);

    vm.setSampleRateIndex(0); // default is 0

    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelState::setFrameSyncEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::settingsChanged);

    vm.setFrameSync("ABCD1234");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.frameSync(), QString("ABCD1234"));
}

void TestMainViewModelState::setFrameSyncNoOpWhenUnchanged()
{
    MainViewModel vm;
    vm.setFrameSync("ABCD1234");

    QSignalSpy spy(&vm, &MainViewModel::settingsChanged);
    vm.setFrameSync("ABCD1234");

    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelState::setPolarityIndexEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::settingsChanged);

    vm.setPolarityIndex(1);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.polarityIndex(), 1);
}

void TestMainViewModelState::setSlopeIndexEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::settingsChanged);

    vm.setSlopeIndex(0); // default is kDefaultSlopeIndex (2)

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.slopeIndex(), 0);
}

void TestMainViewModelState::setScaleEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::settingsChanged);

    vm.setScale("200");

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.scale(), QString("200"));
}

void TestMainViewModelState::setReceiverCountEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::receiverLayoutChanged);

    vm.setReceiverCount(8);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.receiverCount(), 8);
}

void TestMainViewModelState::setReceiverCountResizesGrid()
{
    MainViewModel vm;
    vm.setReceiverCount(4);

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < vm.channelsPerReceiver(); c++)
            QCOMPARE(vm.receiverChecked(r, c), true);

    QCOMPARE(vm.receiverChecked(4, 0), false);
}

void TestMainViewModelState::setChannelsPerReceiverEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::receiverLayoutChanged);

    vm.setChannelsPerReceiver(5);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.channelsPerReceiver(), 5);
}

void TestMainViewModelState::setChannelsPerReceiverResizesGrid()
{
    MainViewModel vm;
    vm.setChannelsPerReceiver(2);

    QCOMPARE(vm.receiverChecked(0, 0), true);
    QCOMPARE(vm.receiverChecked(0, 1), true);
    QCOMPARE(vm.receiverChecked(0, 2), false);
}

void TestMainViewModelState::receiverCheckedValidIndices()
{
    MainViewModel vm;
    QCOMPARE(vm.receiverChecked(0, 0), true);
    QCOMPARE(vm.receiverChecked(15, 2), true);
}

void TestMainViewModelState::receiverCheckedOutOfBoundsReturnsFalse()
{
    MainViewModel vm;
    QCOMPARE(vm.receiverChecked(-1, 0), false);
    QCOMPARE(vm.receiverChecked(0, -1), false);
    QCOMPARE(vm.receiverChecked(100, 0), false);
    QCOMPARE(vm.receiverChecked(0, 100), false);
}

void TestMainViewModelState::setReceiverCheckedEmitsSignal()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::receiverCheckedChanged);

    vm.setReceiverChecked(0, 0, false);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toInt(), 0);
    QCOMPARE(args.at(1).toInt(), 0);
    QCOMPARE(args.at(2).toBool(), false);
    QCOMPARE(vm.receiverChecked(0, 0), false);
}

void TestMainViewModelState::setReceiverCheckedNoOpWhenUnchanged()
{
    MainViewModel vm;
    QSignalSpy spy(&vm, &MainViewModel::receiverCheckedChanged);

    vm.setReceiverChecked(0, 0, true); // default is true

    QCOMPARE(spy.count(), 0);
}

void TestMainViewModelState::setAllReceiversCheckedTrue()
{
    MainViewModel vm;
    vm.setReceiverChecked(0, 0, false);
    vm.setReceiverChecked(1, 1, false);

    vm.setAllReceiversChecked(true);

    for (int r = 0; r < vm.receiverCount(); r++)
        for (int c = 0; c < vm.channelsPerReceiver(); c++)
            QCOMPARE(vm.receiverChecked(r, c), true);
}

void TestMainViewModelState::setAllReceiversCheckedFalse()
{
    MainViewModel vm;

    vm.setAllReceiversChecked(false);

    for (int r = 0; r < vm.receiverCount(); r++)
        for (int c = 0; c < vm.channelsPerReceiver(); c++)
            QCOMPARE(vm.receiverChecked(r, c), false);
}

void TestMainViewModelState::getSettingsDataApplySettingsDataRoundtrip()
{
    MainViewModel vm;

    vm.setFrameSync("DEADBEEF");
    vm.setPolarityIndex(1);
    vm.setSlopeIndex(1);
    vm.setScale("50");
    vm.setExtractAllTime(false);
    vm.setSampleRateIndex(2);
    vm.setReceiverCount(8);
    vm.setChannelsPerReceiver(2);

    SettingsData data = vm.getSettingsData();

    MainViewModel vm2;
    vm2.applySettingsData(data);

    QCOMPARE(vm2.frameSync(), QString("DEADBEEF"));
    QCOMPARE(vm2.polarityIndex(), 1);
    QCOMPARE(vm2.slopeIndex(), 1);
    QCOMPARE(vm2.scale(), QString("50"));
    QCOMPARE(vm2.extractAllTime(), false);
    QCOMPARE(vm2.sampleRateIndex(), 2);
    QCOMPARE(vm2.receiverCount(), 8);
    QCOMPARE(vm2.channelsPerReceiver(), 2);
}

void TestMainViewModelState::applySettingsUpdatesProperties()
{
    MainViewModel vm;

    SettingsData data;
    data.frameSync = "11223344";
    data.polarityIndex = 1;
    data.slopeIndex = 3;
    data.scale = "75";
    data.receiverCount = 4;
    data.channelsPerReceiver = 2;
    data.extractAllTime = vm.extractAllTime();
    data.sampleRateIndex = vm.sampleRateIndex();
    vm.applySettingsData(data);

    QCOMPARE(vm.frameSync(), QString("11223344"));
    QCOMPARE(vm.polarityIndex(), 1);
    QCOMPARE(vm.slopeIndex(), 3);
    QCOMPARE(vm.scale(), QString("75"));
    QCOMPARE(vm.receiverCount(), 4);
    QCOMPARE(vm.channelsPerReceiver(), 2);
}

// v2.0 additions

void TestMainViewModelState::constructorDefaultFrameSync()
{
    MainViewModel vm;
    QCOMPARE(vm.frameSync(), QString(PCMConstants::kDefaultFrameSync));
}

void TestMainViewModelState::lastIniDirDefaultsToSettings()
{
    // Clear any persisted value so the constructor falls back to default
    QSettings app_settings;
    app_settings.remove(UIConstants::kSettingsKeyLastIniDir);
    app_settings.sync();

    MainViewModel vm;
    QVERIFY(vm.lastIniDir().endsWith("/settings"));
}

// v2.0.5 — dynamic frame length

void TestMainViewModelState::loadFrameSetupComputesFrameSizeFromReceiverConfig()
{
    // With default 16 receivers x 3 channels, words_in_frame = 48 + 1 = 49.
    // A parameter at Word=48 (max data word) should be accepted.
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open())
        QSKIP("Could not create temporary file");

    // Write INI content directly to avoid QSettings lock issues on Windows.
    tmp.write("[L_RCVR1]\nWord=48\n");
    tmp.flush();
    tmp.close();

    MainViewModel vm;
    // Default: 16 receivers x 3 channels = 48 data words, frame = 49
    vm.loadFrameSetupFrom(tmp.fileName());
    QCOMPARE(vm.frameSetup()->length(), 1);
    QCOMPARE(vm.frameSetup()->getParameter(0)->word, 47); // stored as 0-based
}

void TestMainViewModelState::loadFrameSetupSmallConfigAcceptsParams()
{
    // With 2 receivers x 1 channel, words_in_frame = 2 + 1 = 3.
    // Word=1 and Word=2 should be accepted; Word=3 would be rejected.
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open())
        QSKIP("Could not create temporary file");

    // Write INI content directly to avoid QSettings lock issues on Windows.
    tmp.write("[L_RCVR1]\nWord=1\n\n[L_RCVR2]\nWord=2\n");
    tmp.flush();
    tmp.close();

    MainViewModel vm;
    vm.setReceiverCount(2);
    vm.setChannelsPerReceiver(1);
    // Frame = 2 data words + 1 = 3

    vm.loadFrameSetupFrom(tmp.fileName());
    QCOMPARE(vm.frameSetup()->length(), 2);
    QCOMPARE(vm.frameSetup()->getParameter(0)->word, 0);
    QCOMPARE(vm.frameSetup()->getParameter(1)->word, 1);
}
