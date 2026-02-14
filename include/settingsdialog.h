/**
 * @file settingsdialog.h
 * @brief Modal dialog for editing PCM frame and receiver settings.
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>

#include "settingsdata.h"

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

    void setData(const SettingsData& data);    ///< Populates all fields from a SettingsData snapshot.
    SettingsData getData() const;              ///< Returns current field values as a SettingsData snapshot.

    void setFrameSync(const QString& value);   ///< Sets the frame sync hex string.
    QString frameSync() const;                  ///< @return Current frame sync hex string.

    void setPolarityIndex(int value);            ///< Sets the polarity combo box index.
    int polarityIndex() const;                  ///< @return Polarity combo box index (0=Positive, 1=Negative).

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
    SettingsData m_data;                 ///< Stored snapshot preserving non-edited fields.
    QLineEdit* m_frame_sync;             ///< Frame sync hex pattern input.
    QComboBox* m_polarity;               ///< Polarity selector (Positive/Negative).
    QComboBox* m_slope;                  ///< Voltage slope selector.
    QLineEdit* m_scale;                  ///< Scale input (dB/V).
    QLineEdit* m_receiver_count;         ///< Receiver count input.
    QLineEdit* m_channels_per_receiver;  ///< Channels-per-receiver input.
};

#endif // SETTINGSDIALOG_H
