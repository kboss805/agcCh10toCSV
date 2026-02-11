#include "settingsdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
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
    m_polarity = new QCheckBox("Negative");
    params_row->addWidget(m_polarity);

    params_row->addWidget(new QLabel("    Scale: "));
    m_scale = new QComboBox;
    m_scale->addItem(QString::fromUtf8("\xc2\xb1") + "10V");
    m_scale->addItem(QString::fromUtf8("\xc2\xb1") + "5V");
    m_scale->addItem("0-10V");
    m_scale->addItem("0-5V");
    params_row->addWidget(m_scale);

    params_row->addWidget(new QLabel("    Range (dB): "));
    m_range = new QLineEdit;
    params_row->addWidget(m_range);

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

void SettingsDialog::setFrameSync(const QString& value)
{
    m_frame_sync->setText(value);
}

QString SettingsDialog::frameSync() const
{
    return m_frame_sync->text();
}

void SettingsDialog::setNegativePolarity(bool value)
{
    m_polarity->setChecked(value);
}

bool SettingsDialog::negativePolarity() const
{
    return m_polarity->isChecked();
}

void SettingsDialog::setScaleIndex(int value)
{
    m_scale->setCurrentIndex(value);
}

int SettingsDialog::scaleIndex() const
{
    return m_scale->currentIndex();
}

void SettingsDialog::setRange(const QString& value)
{
    m_range->setText(value);
}

QString SettingsDialog::range() const
{
    return m_range->text();
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
