#include "mainview.h"

#include <QDebug>
#include <QMessageBox>
#include <QPixmap>
#include <QSignalBlocker>
#include <QTime>

#include "configdialog.h"
#include "constants.h"
#include "mainviewmodel.h"


MainView::MainView(QWidget *parent)
    : QMainWindow(parent),
      m_updating_from_viewmodel(false)
{
    m_view_model = new MainViewModel(this);

    setUpMainLayout();

    setUpConnections();
}

MainView::~MainView()
{
}

////////////////////////////////////////////////////////////////////////////////
//                                 SET UP GUI                                 //
////////////////////////////////////////////////////////////////////////////////

void MainView::setUpMainLayout()
{
    // allocate memory
    m_central_widget = new QWidget;
    m_central_layout = new QHBoxLayout;
    m_controls_layout = new QVBoxLayout;

    // set up constituent parts
    setUpMenuBar();
    setUpTimeChannelRow();
    setUpPCMChannelRow();
    setUpReceiversSection();
    setUpTimeSection();

    m_progress_bar_layout = new QHBoxLayout;
    m_progress_bar = new QProgressBar;
    m_progress_bar->setMinimum(0);
    m_progress_bar->setMaximum(100);
    m_progress_bar->setValue(0);
    m_process_btn = new QPushButton(UIConstants::kButtonTextStart);
    m_process_btn->setObjectName("m_process_btn");
    m_progress_bar_layout->addWidget(m_progress_bar);
    m_progress_bar_layout->addSpacing(16);
    m_progress_bar_layout->addWidget(m_process_btn);

    m_controls_layout->addLayout(m_time_channel_layout);
    m_controls_layout->addLayout(m_pcm_channel_layout);
    m_controls_layout->addWidget(m_receivers_section);
    m_controls_layout->addWidget(m_time_section);
    m_controls_layout->addLayout(m_progress_bar_layout);
    m_controls_layout->addStretch(1);

    m_log_window = new QPlainTextEdit;
    m_log_window->setReadOnly(true);
    m_log_window->setMinimumWidth(300);

    QWidget* controls_widget = new QWidget;
    controls_widget->setLayout(m_controls_layout);
    controls_widget->setFixedWidth(controls_widget->minimumSizeHint().width());

    m_central_layout->addWidget(controls_widget, 0);
    m_central_layout->addWidget(m_log_window, 1);

    // additional settings
    m_central_widget->setLayout(m_central_layout);
    setCentralWidget(m_central_widget);
    setWindowTitle("Chapter 10 to CSV AGC Converter");

    // Initialize UI to disabled state
    m_time_channel->setEnabled(false);
    m_time_channel->clear();
    m_time_channel->addItem("Select a Time Channel");

    m_pcm_channel->setEnabled(false);
    m_pcm_channel->clear();
    m_pcm_channel->addItem("Select a PCM Channel");

    m_receivers_all->setEnabled(false);
    setAllNumberedReceiversEnabled(false);
    m_receivers_all->setChecked(true);
    setAllNumberedReceiversChecked(true);

    m_time_all->setEnabled(false);
    m_time_all->setChecked(true);
    setAllStartStopTimesEnabled(false);
    clearAllStartStopTimes();
    m_sample_rate->setEnabled(false);
    m_sample_rate->setCurrentIndex(0);

    m_progress_bar->setValue(0);
    m_process_btn->setEnabled(false);

    resize(minimumSizeHint());
}

void MainView::setUpMenuBar()
{
    QMenuBar* menu_bar = menuBar();
    QMenu* file_menu = menu_bar->addMenu("&File");

    m_open_file_action = file_menu->addAction("Open Ch10 File...");
    file_menu->addSeparator();
    QAction* settings_action = file_menu->addAction("Settings...");
    file_menu->addSeparator();
    QAction* exit_action = file_menu->addAction("Exit");

    connect(m_open_file_action, &QAction::triggered, this, &MainView::inputFileButtonPressed);
    connect(settings_action, &QAction::triggered, this, &MainView::onSettings);
    connect(exit_action, &QAction::triggered, this, &QMainWindow::close);

    QMenu* help_menu = menu_bar->addMenu("&Help");
    QAction* about_action = help_menu->addAction("About...");
    connect(about_action, &QAction::triggered, this, [this]() {
        QMessageBox about_box(this);
        about_box.setWindowTitle("About");
        about_box.setIconPixmap(QPixmap(":/resources/icon.ico").scaled(
            64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        about_box.setText(
            "<h3>Chapter 10 to CSV AGC Converter</h3>"
            "<p>Version 1.0.0</p>"
            "<p>Extracts PCM data from IRIG 106 Chapter 10 recordings "
            "and exports receiver channel samples to CSV format.</p>");
        about_box.exec();
    });
}

void MainView::setUpTimeChannelRow()
{
    m_time_channel_layout = new QHBoxLayout;

    QLabel* time_label = new QLabel("Time Channel: ");
    m_time_channel = new QComboBox;
    m_time_channel->setMinimumWidth(300);

    m_time_channel_layout->addWidget(time_label);
    m_time_channel_layout->addWidget(m_time_channel);
}

void MainView::setUpPCMChannelRow()
{
    m_pcm_channel_layout = new QHBoxLayout;

    QLabel* pcm_label = new QLabel("PCM Channel: ");
    m_pcm_channel = new QComboBox;
    m_pcm_channel->setMinimumWidth(300);

    m_pcm_channel_layout->addWidget(pcm_label);
    m_pcm_channel_layout->addWidget(m_pcm_channel);
}

void MainView::setUpReceiversSection()
{
    m_receivers_section = new QGroupBox("Receivers");
    m_receivers_section_layout = new QVBoxLayout;
    m_receivers_section->setLayout(m_receivers_section_layout);
    rebuildReceiversGrid();
}

void MainView::rebuildReceiversGrid()
{
    // Clear existing contents
    QLayoutItem* item;
    while ((item = m_receivers_section_layout->takeAt(0)) != nullptr)
    {
        if (item->widget())
            item->widget()->deleteLater();
        if (item->layout())
        {
            QLayoutItem* child;
            while ((child = item->layout()->takeAt(0)) != nullptr)
            {
                if (child->widget())
                    child->widget()->deleteLater();
                if (child->layout())
                {
                    QLayoutItem* grandchild;
                    while ((grandchild = child->layout()->takeAt(0)) != nullptr)
                    {
                        if (grandchild->widget())
                            grandchild->widget()->deleteLater();
                        delete grandchild;
                    }
                }
                delete child;
            }
        }
        delete item;
    }
    m_receiver_checks.clear();

    int receiver_count = m_view_model->receiverCount();
    int channel_count = m_view_model->channelsPerReceiver();

    // Select All checkbox
    m_receivers_all = new QCheckBox("Select All");
    connect(m_receivers_all, &QAbstractButton::toggled, this, &MainView::receiversAllCheckBoxToggled);
    m_receivers_section_layout->addWidget(m_receivers_all);

    // Two side-by-side grids
    QHBoxLayout* grids_row = new QHBoxLayout;
    int per_block = (receiver_count + 1) / 2;

    for (int block = 0; block < 2; block++)
    {
        int start = block * per_block;
        int end = qMin(start + per_block, receiver_count);
        if (start >= receiver_count)
            break;

        QGridLayout* grid = new QGridLayout;
        grid->setHorizontalSpacing(12);

        // Column headers (row 0): blank label column, then channel prefixes
        grid->addWidget(new QLabel(""), 0, 0);
        for (int channel_index = 0; channel_index < channel_count; channel_index++)
            grid->addWidget(new QLabel(MainViewModel::channelPrefix(channel_index)), 0, channel_index + 1, Qt::AlignCenter);

        // Receiver rows
        for (int receiver_index = start; receiver_index < end; receiver_index++)
        {
            int grid_row = receiver_index - start + 1;
            grid->addWidget(new QLabel(QString::number(receiver_index + 1).rightJustified(2)), grid_row, 0);

            QVector<QCheckBox*> row_checkboxes;
            for (int channel_index = 0; channel_index < channel_count; channel_index++)
            {
                QCheckBox* cb = new QCheckBox;
                cb->setChecked(m_view_model->receiverChecked(receiver_index, channel_index));

                // Forward checkbox changes to ViewModel
                connect(cb, &QCheckBox::toggled, this, [this, receiver_index, channel_index](bool checked) {
                    if (!m_updating_from_viewmodel)
                        m_view_model->setReceiverChecked(receiver_index, channel_index, checked);
                });

                grid->addWidget(cb, grid_row, channel_index + 1, Qt::AlignCenter);
                row_checkboxes.append(cb);
            }
            // Insert at the correct index in the outer vector
            if (receiver_index >= m_receiver_checks.size())
                m_receiver_checks.resize(receiver_index + 1);
            m_receiver_checks[receiver_index] = row_checkboxes;
        }

        grids_row->addLayout(grid);
        if (block == 0 && end < receiver_count)
        {
            QSpacerItem* spacer = new QSpacerItem(16, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            grids_row->addItem(spacer);
        }
    }

    m_receivers_section_layout->addLayout(grids_row);
}

void MainView::setUpTimeSection()
{
    // allocate memory
    m_time_section = new QGroupBox("Time");
    QVBoxLayout* time_section_layout = new QVBoxLayout;

    // set up constituent parts
    setUpTimeSectionRow1();
    setUpTimeSectionRow2();
    setUpTimeSectionRow3();

    // add each constituent part
    time_section_layout->addItem(m_time_all_layout);
    time_section_layout->addItem(m_time_start_stop_layout);
    time_section_layout->addItem(m_sample_rate_layout);
    m_time_section->setLayout(time_section_layout);
}

void MainView::setUpTimeSectionRow1()
{
    // allocate memory
    m_time_all_layout = new QHBoxLayout;

    // set up constituent parts
    m_time_all = new QCheckBox("Extract All Time");

    // add each constituent part
    m_time_all_layout->addWidget(m_time_all);
}

void MainView::setUpTimeSectionRow2()
{
    // allocate memory
    m_time_start_stop_layout = new QVBoxLayout;

    // set up constituent parts
    setUpTimeSectionRow2StartTime();
    setUpTimeSectionRow2StopTime();

    // add each constituent part
    m_time_start_stop_layout->addItem(m_time_start_section);
    m_time_start_stop_layout->addItem(m_time_stop_section);
}

void MainView::setUpTimeSectionRow2StartTime()
{
    setUpTimeInputGroup("Start Time:", m_time_start_section,
                        m_start_ddd, m_start_hh,
                        m_start_mm, m_start_ss);
}

void MainView::setUpTimeSectionRow2StopTime()
{
    setUpTimeInputGroup("Stop Time:", m_time_stop_section,
                        m_stop_ddd, m_stop_hh,
                        m_stop_mm, m_stop_ss);
}

void MainView::setUpTimeInputGroup(const QString& label, QVBoxLayout*& section,
                                     QLineEdit*& ddd, QLineEdit*& hh, QLineEdit*& mm, QLineEdit*& ss)
{
    section = new QVBoxLayout;

    QHBoxLayout* header_row = new QHBoxLayout;
    header_row->addWidget(new QLabel(label));

    QGridLayout* input_row = new QGridLayout;
    input_row->setHorizontalSpacing(2);

    int two_char_width = 40;
    int three_char_width = 48;

    ddd = new QLineEdit;
    ddd->setAlignment(Qt::AlignCenter);
    ddd->setMinimumWidth(three_char_width);
    hh = new QLineEdit;
    hh->setAlignment(Qt::AlignCenter);
    hh->setMinimumWidth(two_char_width);
    mm = new QLineEdit;
    mm->setAlignment(Qt::AlignCenter);
    mm->setMinimumWidth(two_char_width);
    ss = new QLineEdit;
    ss->setAlignment(Qt::AlignCenter);
    ss->setMinimumWidth(two_char_width);
    input_row->addWidget(ddd, 0, 0);
    input_row->addWidget(new QLabel(":"), 0, 1);
    input_row->addWidget(hh, 0, 2);
    input_row->addWidget(new QLabel(":"), 0, 3);
    input_row->addWidget(mm, 0, 4);
    input_row->addWidget(new QLabel(":"), 0, 5);
    input_row->addWidget(ss, 0, 6);
    input_row->addWidget(new QLabel("DDD"), 1, 0, Qt::AlignCenter);
    input_row->addWidget(new QLabel("HH"), 1, 2, Qt::AlignCenter);
    input_row->addWidget(new QLabel("MM"), 1, 4, Qt::AlignCenter);
    input_row->addWidget(new QLabel("SS"), 1, 6, Qt::AlignCenter);

    section->addItem(header_row);
    section->addItem(input_row);
}

void MainView::setUpTimeSectionRow3()
{
    // allocate memory
    m_sample_rate_layout = new QHBoxLayout;

    // set up constituent parts
    m_sample_rate = new QComboBox();

    // add each constituent part
    m_sample_rate_layout->addWidget(new QLabel("Sample Rate:"));
    m_sample_rate_layout->addWidget(m_sample_rate);

    // initial settings for parts
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate1Hz) + " Hz");
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate10Hz) + " Hz");
    m_sample_rate->addItem(QString::number(UIConstants::kSampleRate20Hz) + " Hz");
}

////////////////////////////////////////////////////////////////////////////////
//                              GUI CONNECTIONS                               //
////////////////////////////////////////////////////////////////////////////////

void MainView::setUpConnections()
{
    // View -> ViewModel: channel selection
    connect(m_time_channel, &QComboBox::currentIndexChanged,
            m_view_model, &MainViewModel::setTimeChannelIndex);
    connect(m_pcm_channel, &QComboBox::currentIndexChanged,
            m_view_model, &MainViewModel::setPcmChannelIndex);

    // View -> ViewModel: time/sample rate state
    connect(m_time_all, &QAbstractButton::toggled, m_view_model, &MainViewModel::setExtractAllTime);
    connect(m_sample_rate, &QComboBox::currentIndexChanged, m_view_model, &MainViewModel::setSampleRateIndex);

    // View -> View: local UI toggles
    connect(m_time_all, &QAbstractButton::toggled, this, &MainView::timeAllCheckBoxToggled);
    connect(m_process_btn, &QAbstractButton::pressed, this, &MainView::progressProcessButtonPressed);

    // ViewModel -> View: data binding
    connect(m_view_model, &MainViewModel::channelListsChanged, this, &MainView::onChannelListsChanged);
    connect(m_view_model, &MainViewModel::fileLoadedChanged, this, &MainView::onFileLoadedChanged);
    connect(m_view_model, &MainViewModel::fileTimesChanged, this, &MainView::onFileTimesChanged);
    connect(m_view_model, &MainViewModel::progressPercentChanged, this, &MainView::onProgressChanged);
    connect(m_view_model, &MainViewModel::processingChanged, this, &MainView::onProcessingChanged);
    connect(m_view_model, &MainViewModel::receiverLayoutChanged, this, &MainView::onReceiverLayoutChanged);
    connect(m_view_model, &MainViewModel::receiverCheckedChanged, this, &MainView::onReceiverCheckedChanged);
    connect(m_view_model, &MainViewModel::extractAllTimeChanged, this, [this]() {
        QSignalBlocker blocker(m_time_all);
        m_time_all->setChecked(m_view_model->extractAllTime());
        timeAllCheckBoxToggled(m_view_model->extractAllTime());
    });
    connect(m_view_model, &MainViewModel::sampleRateIndexChanged, this, [this]() {
        QSignalBlocker blocker(m_sample_rate);
        m_sample_rate->setCurrentIndex(m_view_model->sampleRateIndex());
    });
    connect(m_view_model, &MainViewModel::errorOccurred, this, &MainView::displayErrorMessage);
    connect(m_view_model, &MainViewModel::processingFinished, this, &MainView::onProcessingFinished);
    connect(m_view_model, &MainViewModel::logMessageReceived, this, &MainView::onLogMessage);
}

////////////////////////////////////////////////////////////////////////////////
//                        VIEWMODEL-DRIVEN SLOTS                              //
////////////////////////////////////////////////////////////////////////////////

void MainView::onChannelListsChanged()
{
    m_updating_from_viewmodel = true;

    m_time_channel->clear();
    m_time_channel->addItem("Select a Time Channel");
    QStringList time_channels = m_view_model->timeChannelList();
    for (const QString& channel : time_channels)
        m_time_channel->addItem(channel);

    m_pcm_channel->clear();
    m_pcm_channel->addItem("Select a PCM Channel");
    QStringList pcm_channels = m_view_model->pcmChannelList();
    for (const QString& channel : pcm_channels)
        m_pcm_channel->addItem(channel);

    m_updating_from_viewmodel = false;
}

void MainView::onFileLoadedChanged()
{
    bool loaded = m_view_model->fileLoaded();

    m_time_channel->setEnabled(loaded);
    m_pcm_channel->setEnabled(loaded);
    m_receivers_all->setEnabled(loaded);
    m_time_all->setEnabled(loaded);
    m_sample_rate->setEnabled(loaded);
    m_process_btn->setEnabled(loaded);

    if (loaded)
    {
        // Auto-select the first channel if none is selected
        if (m_time_channel->currentIndex() == 0 && m_time_channel->count() > 1)
            m_time_channel->setCurrentIndex(1);
        if (m_pcm_channel->currentIndex() == 0 && m_pcm_channel->count() > 1)
            m_pcm_channel->setCurrentIndex(1);
    }
    else
    {
        m_time_channel->setEnabled(false);
        m_time_channel->clear();
        m_time_channel->addItem("Select a Time Channel");

        m_pcm_channel->setEnabled(false);
        m_pcm_channel->clear();
        m_pcm_channel->addItem("Select a PCM Channel");

        m_receivers_all->setEnabled(false);
        setAllNumberedReceiversEnabled(false);
        m_receivers_all->setChecked(true);
        setAllNumberedReceiversChecked(true);

        m_time_all->setEnabled(false);
        m_time_all->setChecked(true);
        setAllStartStopTimesEnabled(false);
        clearAllStartStopTimes();
        m_sample_rate->setEnabled(false);
        m_sample_rate->setCurrentIndex(0);

        m_progress_bar->setValue(0);
        m_process_btn->setEnabled(false);
    }
}

void MainView::onFileTimesChanged()
{
    if (!m_view_model->fileLoaded())
        return;
    fillAllStartStopTimes();
}

void MainView::onProgressChanged()
{
    m_progress_bar->setValue(m_view_model->progressPercent());
}

void MainView::onProcessingChanged()
{
    if (m_view_model->processing())
    {
        setAllControlsEnabled(false);
        m_log_window->clear();
        m_progress_bar->setValue(0);
        m_process_btn->setText(UIConstants::kButtonTextProcessing);
    }
    else
    {
        setAllControlsEnabled(true);
        m_process_btn->setText(UIConstants::kButtonTextStart);
    }
}

void MainView::onProcessingFinished(bool success, const QString& output_file)
{
    if (success)
    {
        m_progress_bar->setValue(100);
        QMessageBox::information(this, "HUZZAH!",
            "Processing complete!\nFile saved to:\n" + output_file);
    }
}

void MainView::onLogMessage(const QString& message)
{
    m_log_window->appendPlainText(
        QTime::currentTime().toString("HH:mm:ss") + "  " + message);
}

void MainView::onReceiverLayoutChanged()
{
    rebuildReceiversGrid();
}

void MainView::onReceiverCheckedChanged(int receiver_index, int channel_index, bool checked)
{
    if (receiver_index < 0 || receiver_index >= m_receiver_checks.size())
        return;
    if (channel_index < 0 || channel_index >= m_receiver_checks[receiver_index].size())
        return;

    m_updating_from_viewmodel = true;
    m_receiver_checks[receiver_index][channel_index]->setChecked(checked);
    m_updating_from_viewmodel = false;

    // Update "Select All" state based on whether all are checked
    bool all_checked = true;
    for (int r = 0; r < m_receiver_checks.size() && all_checked; r++)
        for (int c = 0; c < m_receiver_checks[r].size() && all_checked; c++)
            if (!m_receiver_checks[r][c]->isChecked())
                all_checked = false;

    {
        QSignalBlocker blocker(m_receivers_all);
        m_receivers_all->setChecked(all_checked);
    }
    setAllNumberedReceiversEnabled(!all_checked);
}

////////////////////////////////////////////////////////////////////////////////
//                         USER-INITIATED ACTIONS                             //
////////////////////////////////////////////////////////////////////////////////

void MainView::displayErrorMessage(const QString& message)
{
    QMessageBox::critical(this, "Error", message);
}

void MainView::inputFileButtonPressed()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    m_last_dir,
                                                    tr("Chapter 10 Files (*.ch10);;All Files (*.*)"));

    if (filename.isEmpty())
        return;

    m_last_dir = QFileInfo(filename).absolutePath();
    m_view_model->openFile(filename);
}

void MainView::onSettings()
{
    ConfigDialog dialog(this);
    dialog.setFrameSync(m_view_model->frameSync());
    dialog.setNegativePolarity(m_view_model->negativePolarity());
    dialog.setScaleIndex(m_view_model->scaleIndex());
    dialog.setRange(m_view_model->range());
    dialog.setReceiverCount(m_view_model->receiverCount());
    dialog.setChannelsPerReceiver(m_view_model->channelsPerReceiver());

    connect(&dialog, &ConfigDialog::loadRequested, this, [this, &dialog]() {
        QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                        m_view_model->appRoot() + "/config",
                                                        tr("Configuration Settings Files (*.ini)"));
        if (filename.isEmpty())
            return;

        m_view_model->loadSettings(filename);

        // Update dialog fields from newly loaded config values
        dialog.setFrameSync(m_view_model->frameSync());
        dialog.setNegativePolarity(m_view_model->negativePolarity());
        dialog.setScaleIndex(m_view_model->scaleIndex());
        dialog.setRange(m_view_model->range());
        dialog.setReceiverCount(m_view_model->receiverCount());
        dialog.setChannelsPerReceiver(m_view_model->channelsPerReceiver());
    });

    connect(&dialog, &ConfigDialog::saveAsRequested, this, [this, &dialog]() {
        m_view_model->applyConfig(dialog.frameSync(), dialog.negativePolarity(),
                                   dialog.scaleIndex(), dialog.range(),
                                   dialog.receiverCount(), dialog.channelsPerReceiver());

        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        m_view_model->appRoot() + "/config",
                                                        tr("Configuration Settings Files (*.ini)"));
        if (!filename.isEmpty())
            m_view_model->saveSettings(filename);
    });

    if (dialog.exec() == QDialog::Accepted)
    {
        m_view_model->applyConfig(dialog.frameSync(), dialog.negativePolarity(),
                                   dialog.scaleIndex(), dialog.range(),
                                   dialog.receiverCount(), dialog.channelsPerReceiver());
    }
}

void MainView::receiversAllCheckBoxToggled(bool checked)
{
    m_view_model->setAllReceiversChecked(checked);
    setAllNumberedReceiversChecked(checked);
    setAllNumberedReceiversEnabled(!checked);
}

void MainView::timeAllCheckBoxToggled(bool checked)
{
    if (checked)
    {
        setAllStartStopTimesEnabled(false);
        fillAllStartStopTimes();
    }
    else
    {
        setAllStartStopTimesEnabled(true);
    }
}

void MainView::progressProcessButtonPressed()
{
    // Guard against re-entry
    if (m_view_model->processing())
        return;

    QString outfile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    m_last_dir + "/" + MainViewModel::generateOutputFilename(),
                                                    tr("CSV Files (*.csv);;All Files (*.*)"));
    if (outfile.isEmpty())
        return;

    m_last_dir = QFileInfo(outfile).absolutePath();

    m_view_model->startProcessing(
        outfile,
        m_start_ddd->text(), m_start_hh->text(),
        m_start_mm->text(), m_start_ss->text(),
        m_stop_ddd->text(), m_stop_hh->text(),
        m_stop_mm->text(), m_stop_ss->text(),
        m_sample_rate->currentIndex());
}

////////////////////////////////////////////////////////////////////////////////
//                              HELPER METHODS                                //
////////////////////////////////////////////////////////////////////////////////

void MainView::setAllControlsEnabled(bool enabled)
{
    m_open_file_action->setEnabled(enabled);
    m_time_channel->setEnabled(enabled);
    m_pcm_channel->setEnabled(enabled);
    m_receivers_all->setEnabled(enabled);
    m_time_all->setEnabled(enabled);
    m_sample_rate->setEnabled(enabled);
    m_process_btn->setEnabled(enabled);

    if (enabled)
    {
        if (!m_receivers_all->isChecked())
            setAllNumberedReceiversEnabled(true);
        if (!m_time_all->isChecked())
            setAllStartStopTimesEnabled(true);
    }
    else
    {
        setAllNumberedReceiversEnabled(false);
        setAllStartStopTimesEnabled(false);
    }
}

void MainView::setAllNumberedReceiversEnabled(bool enabled)
{
    for (int receiver_index = 0; receiver_index < m_receiver_checks.size(); receiver_index++)
        for (int channel_index = 0;
             channel_index < m_receiver_checks[receiver_index].size();
             channel_index++)
            m_receiver_checks[receiver_index][channel_index]->setEnabled(enabled);
}

void MainView::setAllNumberedReceiversChecked(bool checked)
{
    for (int receiver_index = 0; receiver_index < m_receiver_checks.size(); receiver_index++)
        for (int channel_index = 0;
             channel_index < m_receiver_checks[receiver_index].size();
             channel_index++)
            m_receiver_checks[receiver_index][channel_index]->setChecked(checked);
}

void MainView::setAllStartStopTimesEnabled(bool enabled)
{
    m_start_ddd->setEnabled(enabled);
    m_start_hh->setEnabled(enabled);
    m_start_mm->setEnabled(enabled);
    m_start_ss->setEnabled(enabled);
    m_stop_ddd->setEnabled(enabled);
    m_stop_hh->setEnabled(enabled);
    m_stop_mm->setEnabled(enabled);
    m_stop_ss->setEnabled(enabled);
}

void MainView::fillAllStartStopTimes()
{
    m_start_ddd->setText(QString::number(m_view_model->startDayOfYear()));
    m_start_hh->setText(QString::number(m_view_model->startHour()));
    m_start_mm->setText(QString::number(m_view_model->startMinute()));
    m_start_ss->setText(QString::number(m_view_model->startSecond()));

    m_stop_ddd->setText(QString::number(m_view_model->stopDayOfYear()));
    m_stop_hh->setText(QString::number(m_view_model->stopHour()));
    m_stop_mm->setText(QString::number(m_view_model->stopMinute()));
    m_stop_ss->setText(QString::number(m_view_model->stopSecond()));
}

void MainView::clearAllStartStopTimes()
{
    m_start_ddd->clear();
    m_start_hh->clear();
    m_start_mm->clear();
    m_start_ss->clear();
    m_stop_ddd->clear();
    m_stop_hh->clear();
    m_stop_mm->clear();
    m_stop_ss->clear();
}
