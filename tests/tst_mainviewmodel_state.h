#ifndef TST_MAINVIEWMODEL_STATE_H
#define TST_MAINVIEWMODEL_STATE_H

#include <QObject>

class TestMainViewModelState : public QObject
{
    Q_OBJECT

private slots:
    void constructorDefaults();
    void setExtractAllTimeEmitsSignal();
    void setExtractAllTimeNoOpWhenUnchanged();
    void setSampleRateIndexEmitsSignal();
    void setSampleRateIndexNoOpWhenUnchanged();
    void setFrameSyncEmitsSignal();
    void setFrameSyncNoOpWhenUnchanged();
    void setPolarityIndexEmitsSignal();
    void setSlopeIndexEmitsSignal();
    void setScaleEmitsSignal();
    void setReceiverCountEmitsSignal();
    void setReceiverCountResizesGrid();
    void setChannelsPerReceiverEmitsSignal();
    void setChannelsPerReceiverResizesGrid();
    void receiverCheckedValidIndices();
    void receiverCheckedOutOfBoundsReturnsFalse();
    void setReceiverCheckedEmitsSignal();
    void setReceiverCheckedNoOpWhenUnchanged();
    void setAllReceiversCheckedTrue();
    void setAllReceiversCheckedFalse();
    void getSettingsDataApplySettingsDataRoundtrip();
    void applySettingsUpdatesProperties();

    // v2.0 additions
    void constructorDefaultFrameSync();
    void lastIniDirDefaultsToSettings();

    // v2.0.5 — dynamic frame length
    void loadFrameSetupComputesFrameSizeFromReceiverConfig();
    void loadFrameSetupSmallConfigAcceptsParams();
};

#endif // TST_MAINVIEWMODEL_STATE_H
