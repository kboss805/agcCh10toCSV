#ifndef TST_SETTINGSMANAGER_H
#define TST_SETTINGSMANAGER_H

#include <QObject>

class TestSettingsManager : public QObject
{
    Q_OBJECT

private slots:
    void loadFileValidIni();
    void loadFileInvalidFrameSync();
    void loadFileInvalidSlope();
    void loadFileInvalidScale();
    void loadFileInvalidReceiverCount();
    void loadFileInvalidChannelsPerReceiver();
    void loadFilePolarityNegative();
    void loadFilePolarityPositive();
    void loadFilePolarityInvalid();
    void loadFileExceedsTotalParameters();
    void saveFileRoundtrip();
    void loadFilePreservesFrameSetup();
    void loadFileParameterCountMismatch();
};

#endif // TST_SETTINGSMANAGER_H
