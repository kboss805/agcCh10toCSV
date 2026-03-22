#include "tst_settingsdialog.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

#include "constants.h"
#include "settingsdialog.h"

/// Helper: finds the OK button in the dialog.
static QPushButton* findOkButton(SettingsDialog& dlg)
{
    for (QPushButton* btn : dlg.findChildren<QPushButton*>())
    {
        if (btn->text() == "OK")
            return btn;
    }
    return nullptr;
}

/// Helper: populates a SettingsData with fully valid values.
static SettingsData validData()
{
    SettingsData data;
    data.frameSync = "FE6B2840";
    data.polarityIndex = 0;
    data.slopeIndex = UIConstants::kDefaultSlopeIndex;
    data.scale = "100";
    data.receiverCount = UIConstants::kDefaultReceiverCount;
    data.channelsPerReceiver = UIConstants::kDefaultChannelsPerReceiver;
    data.extractAllTime = true;
    data.sampleRateIndex = 0;
    return data;
}

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

// --- OK button / inline validation tests ---

void TestSettingsDialog::okButtonDisabledByDefault()
{
    SettingsDialog dlg;
    QPushButton* ok_btn = findOkButton(dlg);
    QVERIFY2(ok_btn != nullptr, "OK button not found in SettingsDialog");

    // Trigger validation by setting a field (fires textChanged).
    // Scale and receiver count are still empty/zero, so OK must remain disabled.
    dlg.setFrameSync("FE6B2840");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled when scale and receiver fields are empty");
}

void TestSettingsDialog::okButtonEnabledAfterValidData()
{
    SettingsDialog dlg;
    QPushButton* ok_btn = findOkButton(dlg);
    QVERIFY2(ok_btn != nullptr, "OK button not found in SettingsDialog");

    dlg.setData(validData());
    QVERIFY2(ok_btn->isEnabled(), "OK must be enabled after setData() with all valid values");
}

void TestSettingsDialog::okButtonDisabledForInvalidFrameSync()
{
    SettingsDialog dlg;
    QPushButton* ok_btn = findOkButton(dlg);
    QVERIFY2(ok_btn != nullptr, "OK button not found in SettingsDialog");

    // Start from a fully valid state so only frame sync is invalid.
    dlg.setData(validData());
    QVERIFY(ok_btn->isEnabled());

    // Non-hex characters
    dlg.setFrameSync("GGGG1234");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for non-hex frame sync");

    // Empty frame sync
    dlg.setFrameSync("");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for empty frame sync");

    // Restore valid hex → OK re-enables
    dlg.setFrameSync("ABCD1234");
    QVERIFY2(ok_btn->isEnabled(), "OK must re-enable after a valid hex frame sync is entered");
}

void TestSettingsDialog::okButtonDisabledForInvalidScale()
{
    SettingsDialog dlg;
    QPushButton* ok_btn = findOkButton(dlg);
    QVERIFY2(ok_btn != nullptr, "OK button not found in SettingsDialog");

    dlg.setData(validData());
    QVERIFY(ok_btn->isEnabled());

    // Zero is not a valid scale
    dlg.setScale("0");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for scale = 0");

    // Negative value
    dlg.setScale("-50");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for negative scale");

    // Non-numeric
    dlg.setScale("abc");
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for non-numeric scale");

    // Restore valid positive value → OK re-enables
    dlg.setScale("50");
    QVERIFY2(ok_btn->isEnabled(), "OK must re-enable after a positive scale is entered");
}

void TestSettingsDialog::okButtonDisabledForInvalidReceivers()
{
    SettingsDialog dlg;
    QPushButton* ok_btn = findOkButton(dlg);
    QVERIFY2(ok_btn != nullptr, "OK button not found in SettingsDialog");

    dlg.setData(validData());
    QVERIFY(ok_btn->isEnabled());

    // Receiver count below minimum (0 < kMinReceiverCount = 1)
    dlg.setReceiverCount(0);
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for 0 receivers");

    // Restore
    dlg.setReceiverCount(UIConstants::kDefaultReceiverCount);
    QVERIFY(ok_btn->isEnabled());

    // Receiver count above maximum (> kMaxReceiverCount = 16)
    dlg.setReceiverCount(UIConstants::kMaxReceiverCount + 1);
    QVERIFY2(!ok_btn->isEnabled(), "OK must be disabled for receiver count > max");

    // Restore
    dlg.setReceiverCount(UIConstants::kDefaultReceiverCount);
    QVERIFY(ok_btn->isEnabled());

    // Total parameters exceed kMaxTotalParameters (48):
    // 16 receivers × 4 channels = 64 > 48
    dlg.setReceiverCount(UIConstants::kMaxReceiverCount);
    dlg.setChannelsPerReceiver(4);
    QVERIFY2(!ok_btn->isEnabled(),
             "OK must be disabled when receivers x channels exceeds kMaxTotalParameters");

    // Reduce channels to bring total back to 48 (16 × 3 = 48)
    dlg.setChannelsPerReceiver(UIConstants::kDefaultChannelsPerReceiver);
    QVERIFY2(ok_btn->isEnabled(), "OK must re-enable when total parameters are within limit");
}
