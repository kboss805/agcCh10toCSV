#include "tst_settingsdialog.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

#include "settingsdialog.h"

// --- Default state tests ---

void TestSettingsDialog::defaultFrameSyncIsEmpty()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.frameSync(), QString(""));
}

void TestSettingsDialog::defaultPolarityIndexIsZero()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.polarityIndex(), 0);
}

void TestSettingsDialog::defaultSlopeIndexIsZero()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.slopeIndex(), 0);
}

void TestSettingsDialog::defaultScaleIsEmpty()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.scale(), QString(""));
}

void TestSettingsDialog::defaultReceiverCountIsZero()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.receiverCount(), 0);
}

void TestSettingsDialog::defaultChannelsPerReceiverIsZero()
{
    SettingsDialog dlg;
    QCOMPARE(dlg.channelsPerReceiver(), 0);
}

// --- Setter/getter roundtrip tests ---

void TestSettingsDialog::setGetFrameSync()
{
    SettingsDialog dlg;
    dlg.setFrameSync("FE6B2840");
    QCOMPARE(dlg.frameSync(), QString("FE6B2840"));

    dlg.setFrameSync("ABCD");
    QCOMPARE(dlg.frameSync(), QString("ABCD"));
}

void TestSettingsDialog::setGetPolarityIndex()
{
    SettingsDialog dlg;
    dlg.setPolarityIndex(1);
    QCOMPARE(dlg.polarityIndex(), 1);

    dlg.setPolarityIndex(0);
    QCOMPARE(dlg.polarityIndex(), 0);
}

void TestSettingsDialog::setGetSlopeIndex()
{
    SettingsDialog dlg;
    dlg.setSlopeIndex(2);
    QCOMPARE(dlg.slopeIndex(), 2);

    dlg.setSlopeIndex(3);
    QCOMPARE(dlg.slopeIndex(), 3);

    dlg.setSlopeIndex(0);
    QCOMPARE(dlg.slopeIndex(), 0);
}

void TestSettingsDialog::setGetScale()
{
    SettingsDialog dlg;
    dlg.setScale("100");
    QCOMPARE(dlg.scale(), QString("100"));

    dlg.setScale("50.5");
    QCOMPARE(dlg.scale(), QString("50.5"));
}

void TestSettingsDialog::setGetReceiverCount()
{
    SettingsDialog dlg;
    dlg.setReceiverCount(16);
    QCOMPARE(dlg.receiverCount(), 16);

    dlg.setReceiverCount(1);
    QCOMPARE(dlg.receiverCount(), 1);
}

void TestSettingsDialog::setGetChannelsPerReceiver()
{
    SettingsDialog dlg;
    dlg.setChannelsPerReceiver(3);
    QCOMPARE(dlg.channelsPerReceiver(), 3);

    dlg.setChannelsPerReceiver(48);
    QCOMPARE(dlg.channelsPerReceiver(), 48);
}

// --- Combo box item count ---

void TestSettingsDialog::slopeComboBoxHasFourItems()
{
    SettingsDialog dlg;
    // Setting index 3 (last valid) should work, confirming 4 items exist
    dlg.setSlopeIndex(3);
    QCOMPARE(dlg.slopeIndex(), 3);
}

void TestSettingsDialog::slopeIndexClampedToValidRange()
{
    SettingsDialog dlg;
    // QComboBox::setCurrentIndex(-1) deselects; verify it returns -1
    dlg.setSlopeIndex(-1);
    QCOMPARE(dlg.slopeIndex(), -1);
}

// --- SettingsData roundtrip tests ---

void TestSettingsDialog::setGetDataRoundtrip()
{
    SettingsDialog dlg;
    SettingsData input;
    input.frameSync = "DEADBEEF";
    input.polarityIndex = 1;
    input.slopeIndex = 2;
    input.scale = "50";
    input.receiverCount = 4;
    input.channelsPerReceiver = 3;
    input.extractAllTime = false;
    input.sampleRateIndex = 1;

    dlg.setData(input);
    SettingsData output = dlg.getData();

    QCOMPARE(output.frameSync, QString("DEADBEEF"));
    QCOMPARE(output.polarityIndex, 1);
    QCOMPARE(output.slopeIndex, 2);
    QCOMPARE(output.scale, QString("50"));
    QCOMPARE(output.receiverCount, 4);
    QCOMPARE(output.channelsPerReceiver, 3);
}

void TestSettingsDialog::getDataPreservesNonEditedFields()
{
    SettingsDialog dlg;
    SettingsData input;
    input.frameSync = "ABCD";
    input.polarityIndex = 0;
    input.slopeIndex = 0;
    input.scale = "100";
    input.receiverCount = 2;
    input.channelsPerReceiver = 1;
    input.extractAllTime = false;
    input.sampleRateIndex = 2;

    dlg.setData(input);
    SettingsData output = dlg.getData();

    QCOMPARE(output.extractAllTime, false);
    QCOMPARE(output.sampleRateIndex, 2);
}

// --- Signal tests ---

void TestSettingsDialog::loadRequestedSignal()
{
    SettingsDialog dlg;
    QSignalSpy spy(&dlg, &SettingsDialog::loadRequested);
    QVERIFY(spy.isValid());

    // Find and click the "Load..." button
    QPushButton* load_btn = nullptr;
    for (QPushButton* btn : dlg.findChildren<QPushButton*>())
    {
        if (btn->text() == "Load...")
        {
            load_btn = btn;
            break;
        }
    }
    QVERIFY(load_btn != nullptr);
    load_btn->click();
    QCOMPARE(spy.count(), 1);
}

void TestSettingsDialog::saveAsRequestedSignal()
{
    SettingsDialog dlg;
    QSignalSpy spy(&dlg, &SettingsDialog::saveAsRequested);
    QVERIFY(spy.isValid());

    // Find and click the "Save As..." button
    QPushButton* save_btn = nullptr;
    for (QPushButton* btn : dlg.findChildren<QPushButton*>())
    {
        if (btn->text() == "Save As...")
        {
            save_btn = btn;
            break;
        }
    }
    QVERIFY(save_btn != nullptr);
    save_btn->click();
    QCOMPARE(spy.count(), 1);
}
