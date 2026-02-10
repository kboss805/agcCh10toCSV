/**
 * @file configdialog.h
 * @brief Modal dialog for editing PCM frame and receiver configuration.
 */

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>

/**
 * @brief Modal dialog that lets the user edit frame sync, scale, polarity,
 *        range, and receiver layout settings.
 *
 * Emits loadRequested() and saveAsRequested() so the caller can handle
 * file I/O through SettingsManager.
 */
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget* parent = nullptr);

    void setFrameSync(const QString& value);   ///< Sets the frame sync hex string.
    QString frameSync() const;                  ///< @return Current frame sync hex string.

    void setNegativePolarity(bool value);       ///< Sets the polarity checkbox state.
    bool negativePolarity() const;              ///< @return True if negative polarity is selected.

    void setScaleIndex(int value);              ///< Sets the voltage scale combo box index.
    int scaleIndex() const;                     ///< @return Voltage scale combo box index.

    void setRange(const QString& value);        ///< Sets the range in dB.
    QString range() const;                      ///< @return Range in dB as a string.

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
    QComboBox* m_scale;                  ///< Voltage scale selector.
    QLineEdit* m_range;                  ///< Full-scale range input (dB).
    QLineEdit* m_receiver_count;         ///< Receiver count input.
    QLineEdit* m_channels_per_receiver;  ///< Channels-per-receiver input.
};

#endif // CONFIGDIALOG_H
