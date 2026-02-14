/**
 * @file settingsdialog.cpp
 * @brief Implementation of SettingsDialog — modal settings editor.
 */

#include "settingsdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent), m_data{}
{
    setWindowTitle("Settings");

    QVBoxLayout* main_layout = new QVBoxLayout;

    // Frame group
    QGroupBox* frame_group = new QGroupBox("Frame");
    QVBoxLayout* frame_layout = new QVBoxLayout;

    QHBoxLayout* sync_row = new QHBoxLayout;
    sync_row->addWidget(new QLabel("Frame Sync: "));
    m_frame_sync = new QLineEdit;
    sync_row->addWidget(m_frame_sync);

    frame_layout->addLayout(sync_row);
    frame_group->setLayout(frame_layout);

    // Parameters group
    QGroupBox* params_group = new QGroupBox("Parameters");
    QVBoxLayout* params_layout = new QVBoxLayout;

    QHBoxLayout* params_row = new QHBoxLayout;
    params_row->addWidget(new QLabel("Polarity: "));
    m_polarity = new QComboBox;
    m_polarity->addItem("Positive");
    m_polarity->addItem("Negative");
    params_row->addWidget(m_polarity);

    params_row->addWidget(new QLabel("    Slope: "));
    m_slope = new QComboBox;
    m_slope->addItem(QString::fromUtf8("\xc2\xb1") + "10V");
    m_slope->addItem(QString::fromUtf8("\xc2\xb1") + "5V");
    m_slope->addItem("0-10V");
    m_slope->addItem("0-5V");
    params_row->addWidget(m_slope);

    params_row->addWidget(new QLabel("    Scale (dB/V): "));
    m_scale = new QLineEdit;
    params_row->addWidget(m_scale);

    params_layout->addLayout(params_row);
    params_group->setLayout(params_layout);

    // Receivers group
    QGroupBox* receivers_group = new QGroupBox("Receivers");
    QVBoxLayout* receivers_layout = new QVBoxLayout;

    QHBoxLayout* rcvr_count_row = new QHBoxLayout;
    rcvr_count_row->addWidget(new QLabel("Number of Receivers: "));
    m_receiver_count = new QLineEdit;
    rcvr_count_row->addWidget(m_receiver_count);

    QHBoxLayout* channels_row = new QHBoxLayout;
    channels_row->addWidget(new QLabel("Channels per Receiver: "));
    m_channels_per_receiver = new QLineEdit;
    channels_row->addWidget(m_channels_per_receiver);

    receivers_layout->addLayout(rcvr_count_row);
    receivers_layout->addLayout(channels_row);
    receivers_group->setLayout(receivers_layout);

    // Button box
    QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton* load_button = button_box->addButton("Load...", QDialogButtonBox::ActionRole);
    QPushButton* save_as_button = button_box->addButton("Save As...", QDialogButtonBox::ActionRole);
    connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    main_layout->addWidget(button_box);
    setLayout(main_layout);
}

void SettingsDialog::setData(const SettingsData& data)
{
    m_data = data;
    m_frame_sync->setText(data.frameSync);
    m_polarity->setCurrentIndex(data.polarityIndex);
    m_slope->setCurrentIndex(data.slopeIndex);
    m_scale->setText(data.scale);
    m_receiver_count->setText(QString::number(data.receiverCount));
    m_channels_per_receiver->setText(QString::number(data.channelsPerReceiver));
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
