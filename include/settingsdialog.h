/**
 * @file settingsdialog.h
 * @brief Modal dialog for editing PCM frame and receiver settings.
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>

/**
 * @brief Modal dialog that lets the user edit frame sync, slope, polarity,
 *        scale, and receiver layout settings.
 *
 * Emits loadRequested() and saveAsRequested() so the caller can handle
 * file I/O through SettingsManager.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent = nullptr);

    void setFrameSync(const QString& value);   ///< Sets the frame sync hex string.
    QString frameSync() const;                  ///< @return Current frame sync hex string.

    void setNegativePolarity(bool value);       ///< Sets the polarity checkbox state.
    bool negativePolarity() const;              ///< @return True if negative polarity is selected.

    void setSlopeIndex(int value);              ///< Sets the voltage slope combo box index.
    int slopeIndex() const;                     ///< @return Voltage slope combo box index.

    void setScale(const QString& value);        ///< Sets the scale in dB/V.
    QString scale() const;                      ///< @return Scale in dB/V as a string.

    void setReceiverCount(int value);           ///< Sets the number of receivers.
    int receiverCount() const;                  ///< @return Number of receivers.

    void setChannelsPerReceiver(int value);     ///< Sets channels per receiver.
    int channelsPerReceiver() const;            ///< @return Channels per receiver.

signals:
    void loadRequested();    ///< Emitted when the user clicks "Load...".
    void saveAsRequested();  ///< Emitted when the user clicks "Save As...".

private:
    QLineEdit* m_frame_sync;             ///< Frame sync hex pattern input.
    QCheckBox* m_polarity;               ///< Negative-polarity toggle.
    QComboBox* m_slope;                  ///< Voltage slope selector.
    QLineEdit* m_scale;                  ///< Scale input (dB/V).
    QLineEdit* m_receiver_count;         ///< Receiver count input.
    QLineEdit* m_channels_per_receiver;  ///< Channels-per-receiver input.
};

#endif // SETTINGSDIALOG_H
