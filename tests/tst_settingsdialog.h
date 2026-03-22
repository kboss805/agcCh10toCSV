#ifndef TST_SETTINGSDIALOG_H
#define TST_SETTINGSDIALOG_H

#include <QObject>

class TestSettingsDialog : public QObject
{
    Q_OBJECT

private slots:
    void defaultFrameSyncIsEmpty();
    void defaultPolarityIndexIsZero();
    void defaultSlopeIndexIsZero();
    void defaultScaleIsEmpty();
    void defaultReceiverCountIsZero();
    void defaultChannelsPerReceiverIsZero();

    void setGetFrameSync();
    void setGetPolarityIndex();
    void setGetSlopeIndex();
    void setGetScale();
    void setGetReceiverCount();
    void setGetChannelsPerReceiver();

    void slopeComboBoxHasFourItems();
    void slopeIndexClampedToValidRange();

    void setGetDataRoundtrip();
    void getDataPreservesNonEditedFields();

    void loadRequestedSignal();
    void saveAsRequestedSignal();

    // v3.2 — OK button / inline validation
    void okButtonDisabledByDefault();
    void okButtonEnabledAfterValidData();
    void okButtonDisabledForInvalidFrameSync();
    void okButtonDisabledForInvalidScale();
    void okButtonDisabledForInvalidReceivers();
};

#endif // TST_SETTINGSDIALOG_H
