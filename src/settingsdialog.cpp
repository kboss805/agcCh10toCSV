/**
 * @file settingsdialog.cpp
 * @brief Implementation of SettingsDialog — modal settings editor.
 */

#include "settingsdialog.h"

#include <QHBoxLayout>
#include "constants.h"
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent),
      m_data{},
      m_frame_sync(new QLineEdit),
      m_polarity(new QComboBox),
      m_slope(new QComboBox),
      m_scale(new QLineEdit),
      m_receiver_count(new QLineEdit),
      m_channels_per_receiver(new QLineEdit),
      m_sync_error_label(new QLabel),
      m_scale_error_label(new QLabel),
      m_receivers_error_label(new QLabel),
      m_ok_button(new QPushButton("OK"))
{
    setWindowTitle("Settings");

    QVBoxLayout* main_layout = new QVBoxLayout;

    // Frame group
    QGroupBox* frame_group = new QGroupBox("Frame");
    QVBoxLayout* frame_layout = new QVBoxLayout;

    QHBoxLayout* sync_row = new QHBoxLayout;
    sync_row->addWidget(new QLabel("Frame Sync: "));
    sync_row->addWidget(m_frame_sync);

    m_sync_error_label->setStyleSheet("color: red;");
    m_sync_error_label->setVisible(false);

    frame_layout->addLayout(sync_row);
    frame_layout->addWidget(m_sync_error_label);
    frame_group->setLayout(frame_layout);

    // Parameters group
    QGroupBox* params_group = new QGroupBox("Parameters");
    QVBoxLayout* params_layout = new QVBoxLayout;

    QHBoxLayout* params_row = new QHBoxLayout;
    params_row->addWidget(new QLabel("Polarity: "));
    m_polarity->addItem("Positive");
    m_polarity->addItem("Negative");
    params_row->addWidget(m_polarity);

    params_row->addWidget(new QLabel("    Slope: "));
    m_slope->addItem(QString::fromUtf8("\xc2\xb1") + "10V");
    m_slope->addItem(QString::fromUtf8("\xc2\xb1") + "5V");
    m_slope->addItem("0-10V");
    m_slope->addItem("0-5V");
    params_row->addWidget(m_slope);

    params_row->addWidget(new QLabel("    Scale (dB/V): "));
    params_row->addWidget(m_scale);

    m_scale_error_label->setStyleSheet("color: red;");
    m_scale_error_label->setVisible(false);

    params_layout->addLayout(params_row);
    params_layout->addWidget(m_scale_error_label);
    params_group->setLayout(params_layout);

    // Receivers group
    QGroupBox* receivers_group = new QGroupBox("Receivers");
    QVBoxLayout* receivers_layout = new QVBoxLayout;

    QHBoxLayout* rcvr_count_row = new QHBoxLayout;
    rcvr_count_row->addWidget(new QLabel("Number of Receivers: "));
    rcvr_count_row->addWidget(m_receiver_count);

    QHBoxLayout* channels_row = new QHBoxLayout;
    channels_row->addWidget(new QLabel("Channels per Receiver: "));
    channels_row->addWidget(m_channels_per_receiver);

    m_receivers_error_label->setStyleSheet("color: red;");
    m_receivers_error_label->setVisible(false);

    receivers_layout->addLayout(rcvr_count_row);
    receivers_layout->addLayout(channels_row);
    receivers_layout->addWidget(m_receivers_error_label);
    receivers_group->setLayout(receivers_layout);

    // Button row: Load... | Save As... | <stretch> | Cancel | OK
    QHBoxLayout* button_row = new QHBoxLayout;
    QPushButton* load_button = new QPushButton("Load...");
    QPushButton* save_as_button = new QPushButton("Save As...");
    QPushButton* cancel_button = new QPushButton("Cancel");
    m_ok_button->setDefault(true);
    button_row->addWidget(load_button);
    button_row->addWidget(save_as_button);
    button_row->addStretch();
    button_row->addWidget(cancel_button);
    button_row->addWidget(m_ok_button);
    connect(m_ok_button, &QPushButton::clicked, this, &SettingsDialog::validateAndAccept);
    connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_frame_sync, &QLineEdit::textChanged, this, [this]() { validateSync(); });
    connect(m_scale, &QLineEdit::textChanged, this, [this]() { validateScale(); });
    connect(m_receiver_count, &QLineEdit::textChanged, this, [this]() { validateReceivers(); });
    connect(m_channels_per_receiver, &QLineEdit::textChanged, this, [this]() { validateReceivers(); });
    connect(load_button, &QPushButton::clicked, this, [this]() {
        emit loadRequested();
    });
    connect(save_as_button, &QPushButton::clicked, this, [this]() {
        emit saveAsRequested();
        accept();
    });

    main_layout->addWidget(frame_group);
    main_layout->addWidget(params_group);
    main_layout->addWidget(receivers_group);
    main_layout->addLayout(button_row);
    setLayout(main_layout);
}

void SettingsDialog::updateOkButton()
{
    static const QRegularExpression hex_pattern("^[0-9A-Fa-f]+$");
    const QString sync = m_frame_sync->text().trimmed();
    const bool sync_valid = !sync.isEmpty() && hex_pattern.match(sync).hasMatch();

    bool scale_ok = false;
    const double scale_value = m_scale->text().trimmed().toDouble(&scale_ok);
    const bool scale_valid = scale_ok && scale_value > 0.0;

    bool rcvr_ok = false, ch_ok = false;
    const int rcvr = m_receiver_count->text().trimmed().toInt(&rcvr_ok);
    const int ch   = m_channels_per_receiver->text().trimmed().toInt(&ch_ok);
    const bool receivers_valid = rcvr_ok && ch_ok
        && rcvr >= UIConstants::kMinReceiverCount && rcvr <= UIConstants::kMaxReceiverCount
        && ch   >= UIConstants::kMinChannelsPerReceiver && ch <= UIConstants::kMaxChannelsPerReceiver
        && (rcvr * ch) <= UIConstants::kMaxTotalParameters;

    m_ok_button->setEnabled(sync_valid && scale_valid && receivers_valid);
}

bool SettingsDialog::validateSync()
{
    static const QRegularExpression hex_pattern("^[0-9A-Fa-f]+$");
    const QString sync = m_frame_sync->text().trimmed();
    const bool valid = !sync.isEmpty() && hex_pattern.match(sync).hasMatch();
    m_frame_sync->setStyleSheet(valid ? "" : "border: 1px solid red;");
    m_sync_error_label->setText("Frame Sync must contain only hexadecimal characters (0–9, A–F).");
    m_sync_error_label->setVisible(!valid);
    updateOkButton();
    return valid;
}

bool SettingsDialog::validateScale()
{
    bool ok = false;
    const double value = m_scale->text().trimmed().toDouble(&ok);
    const bool valid = ok && value > 0.0;
    m_scale->setStyleSheet(valid ? "" : "border: 1px solid red;");
    m_scale_error_label->setText("Scale must be a positive number greater than zero.");
    m_scale_error_label->setVisible(!valid);
    updateOkButton();
    return valid;
}

bool SettingsDialog::validateReceivers()
{
    bool rcvr_ok = false, ch_ok = false;
    const int rcvr = m_receiver_count->text().trimmed().toInt(&rcvr_ok);
    const int ch   = m_channels_per_receiver->text().trimmed().toInt(&ch_ok);

    const bool rcvr_in_range = rcvr_ok
        && rcvr >= UIConstants::kMinReceiverCount
        && rcvr <= UIConstants::kMaxReceiverCount;
    const bool ch_in_range = ch_ok
        && ch >= UIConstants::kMinChannelsPerReceiver
        && ch <= UIConstants::kMaxChannelsPerReceiver;
    const bool total_ok = rcvr_in_range && ch_in_range
        && (rcvr * ch) <= UIConstants::kMaxTotalParameters;

    const bool valid = rcvr_in_range && ch_in_range && total_ok;

    m_receiver_count->setStyleSheet(rcvr_in_range ? "" : "border: 1px solid red;");
    m_channels_per_receiver->setStyleSheet(ch_in_range ? "" : "border: 1px solid red;");

    if (!rcvr_in_range) {
        m_receivers_error_label->setText(
            QString("Number of Receivers must be between %1 and %2.")
                .arg(UIConstants::kMinReceiverCount)
                .arg(UIConstants::kMaxReceiverCount));
    } else if (!ch_in_range) {
        m_receivers_error_label->setText(
            QString("Channels per Receiver must be between %1 and %2.")
                .arg(UIConstants::kMinChannelsPerReceiver)
                .arg(UIConstants::kMaxChannelsPerReceiver));
    } else if (!total_ok) {
        m_receivers_error_label->setText(
            QString("Receivers × Channels (%1) exceeds the maximum of %2 words.")
                .arg(rcvr * ch)
                .arg(UIConstants::kMaxTotalParameters));
    }
    m_receivers_error_label->setVisible(!valid);
    updateOkButton();
    return valid;
}

void SettingsDialog::validateAndAccept()
{
    const bool sync_valid      = validateSync();
    const bool scale_valid     = validateScale();
    const bool receivers_valid = validateReceivers();
    if (sync_valid && scale_valid && receivers_valid)
        accept();
}

void SettingsDialog::setData(const SettingsData& data)
{
    m_data = data;
    m_frame_sync->setText(data.frameSync);  // triggers validateSync() via textChanged
    m_polarity->setCurrentIndex(data.polarityIndex);
    m_slope->setCurrentIndex(data.slopeIndex);
    m_scale->setText(data.scale);           // triggers validateScale() via textChanged
    m_receiver_count->setText(QString::number(data.receiverCount));         // triggers validateReceivers() via textChanged
    m_channels_per_receiver->setText(QString::number(data.channelsPerReceiver)); // triggers validateReceivers() via textChanged
}

SettingsData SettingsDialog::getData() const
{
    SettingsData data = m_data;
    data.frameSync = m_frame_sync->text();
    data.polarityIndex = m_polarity->currentIndex();
    data.slopeIndex = m_slope->currentIndex();
    data.scale = m_scale->text();
    data.receiverCount = m_receiver_count->text().toInt();
    data.channelsPerReceiver = m_channels_per_receiver->text().toInt();
    return data;
}

void SettingsDialog::setFrameSync(const QString& value)
{
    m_frame_sync->setText(value);
}

QString SettingsDialog::frameSync() const
{
    return m_frame_sync->text();
}

void SettingsDialog::setPolarityIndex(int value)
{
    m_polarity->setCurrentIndex(value);
}

int SettingsDialog::polarityIndex() const
{
    return m_polarity->currentIndex();
}

void SettingsDialog::setSlopeIndex(int value)
{
    m_slope->setCurrentIndex(value);
}

int SettingsDialog::slopeIndex() const
{
    return m_slope->currentIndex();
}

void SettingsDialog::setScale(const QString& value)
{
    m_scale->setText(value);
}

QString SettingsDialog::scale() const
{
    return m_scale->text();
}

void SettingsDialog::setReceiverCount(int value)
{
    m_receiver_count->setText(QString::number(value));
}

int SettingsDialog::receiverCount() const
{
    return m_receiver_count->text().toInt();
}

void SettingsDialog::setChannelsPerReceiver(int value)
{
    m_channels_per_receiver->setText(QString::number(value));
}

int SettingsDialog::channelsPerReceiver() const
{
    return m_channels_per_receiver->text().toInt();
}
