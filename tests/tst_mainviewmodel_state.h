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
    void setNegativePolarityEmitsSignal();
    void setScaleIndexEmitsSignal();
    void setRangeEmitsSignal();
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
};

#endif // TST_MAINVIEWMODEL_STATE_H
