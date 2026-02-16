/**
 * @file timeextractionwidget.cpp
 * @brief Implementation of TimeExtractionWidget — time range and sample rate controls.
 */

#include "timeextractionwidget.h"

#include <QChar>
#include <QString>

TimeExtractionWidget::TimeExtractionWidget(QWidget* parent)
    : QWidget(parent)
{
    QGridLayout* time_grid = new QGridLayout;
    time_grid->setContentsMargins(0, 0, 0, 0);
    time_grid->setVerticalSpacing(2);

    m_time_all = new QCheckBox("Extract All Time");
    m_sample_rate = new QComboBox;
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate1Hz) + " Hz");
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate10Hz) + " Hz");
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate100Hz) + " Hz");

    m_start_time = new QLineEdit;
    m_start_time->setInputMask("000:00:00:00;_");
    m_start_time->setPlaceholderText("DDD:HH:MM:SS");
    m_start_time->setMaximumWidth(100);

    m_stop_time = new QLineEdit;
    m_stop_time->setInputMask("000:00:00:00;_");
    m_stop_time->setPlaceholderText("DDD:HH:MM:SS");
    m_stop_time->setMaximumWidth(100);

    // Row 0: Extract All Time (spans 0-1) | <stretch> | Sample Rate: | combo
    // Row 1: Start | start input          | <stretch> | Stop         | stop input
    time_grid->addWidget(m_time_all,                0, 0, 1, 2);
    time_grid->addWidget(new QLabel("Sample Rate"), 0, 3, Qt::AlignRight);
    time_grid->addWidget(m_sample_rate,             0, 4);
    time_grid->addWidget(new QLabel("Start"),       1, 0);
    time_grid->addWidget(m_start_time,              1, 1);
    time_grid->addWidget(new QLabel("Stop"),        1, 3, Qt::AlignRight);
    time_grid->addWidget(m_stop_time,               1, 4, Qt::AlignLeft);

    time_grid->setColumnStretch(2, 1);
    setLayout(time_grid);

    // Internal wiring: toggling "Extract All" enables/disables the time fields
    connect(m_time_all, &QAbstractButton::toggled, this, [this](bool checked) {
        if (checked)
            fillTimes(0, 0, 0, 0, 0, 0, 0, 0);  // Will be refilled by the parent when file is loaded
        m_start_time->setEnabled(!checked);
        m_stop_time->setEnabled(!checked);
        emit extractAllTimeChanged(checked);
    });

    connect(m_sample_rate, &QComboBox::currentIndexChanged,
            this, &TimeExtractionWidget::sampleRateIndexChanged);
}

bool TimeExtractionWidget::extractAllTime() const
{
    return m_time_all->isChecked();
}

void TimeExtractionWidget::setExtractAllTime(bool value)
{
    QSignalBlocker blocker(m_time_all);
    m_time_all->setChecked(value);
    m_start_time->setEnabled(!value);
    m_stop_time->setEnabled(!value);
}

int TimeExtractionWidget::sampleRateIndex() const
{
    return m_sample_rate->currentIndex();
}

void TimeExtractionWidget::setSampleRateIndex(int index)
{
    QSignalBlocker blocker(m_sample_rate);
    m_sample_rate->setCurrentIndex(index);
}

void TimeExtractionWidget::setAllEnabled(bool enabled)
{
    m_time_all->setEnabled(enabled);
    m_sample_rate->setEnabled(enabled);

    if (enabled)
    {
        if (!m_time_all->isChecked())
        {
            m_start_time->setEnabled(true);
            m_stop_time->setEnabled(true);
        }
    }
    else
    {
        m_start_time->setEnabled(false);
        m_stop_time->setEnabled(false);
    }
}

void TimeExtractionWidget::setSampleRateEnabled(bool enabled)
{
    m_sample_rate->setEnabled(enabled);
}

void TimeExtractionWidget::fillTimes(int start_ddd, int start_hh, int start_mm, int start_ss,
                                      int stop_ddd, int stop_hh, int stop_mm, int stop_ss)
{
    m_start_time->setText(
        QString("%1:%2:%3:%4")
            .arg(start_ddd, 3, 10, QChar('0'))
            .arg(start_hh, 2, 10, QChar('0'))
            .arg(start_mm, 2, 10, QChar('0'))
            .arg(start_ss, 2, 10, QChar('0')));

    m_stop_time->setText(
        QString("%1:%2:%3:%4")
            .arg(stop_ddd, 3, 10, QChar('0'))
            .arg(stop_hh, 2, 10, QChar('0'))
            .arg(stop_mm, 2, 10, QChar('0'))
            .arg(stop_ss, 2, 10, QChar('0')));
}

void TimeExtractionWidget::clearTimes()
{
    m_start_time->clear();
    m_stop_time->clear();
}

QString TimeExtractionWidget::startTimeText() const
{
    return m_start_time->text();
}

QString TimeExtractionWidget::stopTimeText() const
{
    return m_stop_time->text();
}
