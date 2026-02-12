/**
 * @file mainview.cpp
 * @brief Implementation of MainView — Qt Widgets UI and ViewModel signal wiring.
 */

#include "mainview.h"

#include <QApplication>
#include <QDebug>
#include <QGridLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QSignalBlocker>
#include <QTime>

#include "settingsdialog.h"
#include "constants.h"
#include "mainviewmodel.h"


MainView::MainView(QWidget *parent)
    : QMainWindow(parent),
      m_updating_from_viewmodel(false)
{
    m_view_model = new MainViewModel(this);

    setAcceptDrops(true);

    setUpMainLayout();

    QSettings app_settings(UIConstants::kOrganizationName, UIConstants::kApplicationName);
    m_last_dir = app_settings.value(UIConstants::kSettingsKeyLastDir).toString();

    setUpConnections();
}

MainView::~MainView()
{
}

void MainView::saveLastDir()
{
    QSettings app_settings(UIConstants::kOrganizationName, UIConstants::kApplicationName);
    app_settings.setValue(UIConstants::kSettingsKeyLastDir, m_last_dir);
}

////////////////////////////////////////////////////////////////////////////////
//                                 SET UP GUI                                 //
////////////////////////////////////////////////////////////////////////////////

void MainView::setUpMainLayout()
{
    m_controls_layout = new QVBoxLayout;

    // set up constituent parts
    setUpMenuBar();
    setUpTimeChannelRow();
    setUpPCMChannelRow();
    setUpReceiversSection();
    setUpTimeSection();

    m_progress_bar = new QProgressBar;
    m_progress_bar->setMinimum(0);
    m_progress_bar->setMaximum(100);
    m_progress_bar->setValue(0);

    m_input_file = new QLineEdit;
    m_input_file->setReadOnly(true);
    m_input_file->setPlaceholderText("No file loaded");

    m_controls_layout->addWidget(m_input_file);
    m_controls_layout->addLayout(m_time_channel_layout);
    m_controls_layout->addLayout(m_pcm_channel_layout);
    m_controls_layout->addWidget(m_receivers_section);
    m_controls_layout->addWidget(m_time_section);
    m_controls_layout->addWidget(m_progress_bar);
    m_controls_layout->addStretch(1);

    // Log is the central widget so it stretches on resize
    m_log_window = new QPlainTextEdit;
    m_log_window->setReadOnly(true);
    m_log_window->setMinimumWidth(UIConstants::kLogMinimumWidth);
    setCentralWidget(m_log_window);

    // Controls in a fixed left dock widget
    QWidget* controls_widget = new QWidget;
    controls_widget->setLayout(m_controls_layout);
    controls_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    m_controls_dock = new QDockWidget("Controls", this);
    m_controls_dock->setWidget(controls_widget);
    m_controls_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, m_controls_dock);

    // additional settings
    setWindowTitle("Chapter 10 to CSV AGC Converter");

    // Initialize UI to disabled state
    m_time_channel->setEnabled(false);
    m_time_channel->clear();
    m_time_channel->addItem("Select a Time Channel");

    m_pcm_channel->setEnabled(false);
    m_pcm_channel->clear();
    m_pcm_channel->addItem("Select a PCM Channel");

    setAllNumberedReceiversEnabled(false);
    setAllNumberedReceiversChecked(true);

    m_time_all->setEnabled(false);
    m_time_all->setChecked(true);
    setAllStartStopTimesEnabled(false);
    clearAllStartStopTimes();
    m_sample_rate->setEnabled(false);
    m_sample_rate->setCurrentIndex(0);

    m_progress_bar->setValue(0);

    resize(minimumSizeHint());
}

void MainView::setUpMenuBar()
{
    QMenuBar* menu_bar = menuBar();
    QMenu* file_menu = menu_bar->addMenu("&File");

    QAction* settings_action = file_menu->addAction("Settings...");
    file_menu->addSeparator();

    QSettings app_settings(UIConstants::kOrganizationName, UIConstants::kApplicationName);
    QString current_theme = app_settings.value(UIConstants::kSettingsKeyTheme, UIConstants::kThemeDark).toString();
    m_theme_action = file_menu->addAction(
        (current_theme == UIConstants::kThemeDark) ? "Switch to Light Theme" : "Switch to Dark Theme");
    file_menu->addSeparator();

    QAction* exit_action = file_menu->addAction("Exit");

    connect(settings_action, &QAction::triggered, this, &MainView::onSettings);
    connect(m_theme_action, &QAction::triggered, this, &MainView::onToggleTheme);
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
            "<p>Version " + AppVersion::toString() + "</p>"
            "<p>Extracts PCM data from IRIG 106 Chapter 10 recordings "
            "and exports receiver channel samples to CSV format.</p>");
        about_box.exec();
    });

    // Toolbar
    m_toolbar = addToolBar("Main");
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolbar->setIconSize(QSize(24, 24));

    m_toolbar_open_action = m_toolbar->addAction(
        QIcon(":/resources/folder-open.svg"), "Open Ch10 File");
    m_toolbar_open_action->setToolTip("Open Ch10 File");
    connect(m_toolbar_open_action, &QAction::triggered,
            this, &MainView::inputFileButtonPressed);

    m_toolbar->addSeparator();

    m_process_action = m_toolbar->addAction(
        QIcon(":/resources/play.svg"), "Process");
    m_process_action->setToolTip("Process Ch10 to CSV");
    m_process_action->setEnabled(false);
    connect(m_process_action, &QAction::triggered,
            this, &MainView::progressProcessButtonPressed);
}

void MainView::setUpTimeChannelRow()
{
    m_time_channel_layout = new QHBoxLayout;
    m_time_channel_layout->addWidget(new QLabel("Time Channel"));
    m_time_channel = new QComboBox;
    m_time_channel_layout->addWidget(m_time_channel, 1);
}

void MainView::setUpPCMChannelRow()
{
    m_pcm_channel_layout = new QHBoxLayout;
    m_pcm_channel_layout->addWidget(new QLabel("PCM Channel"));
    m_pcm_channel = new QComboBox;
    m_pcm_channel_layout->addWidget(m_pcm_channel, 1);
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
                delete child;
            }
        }
        delete item;
    }
    m_receiver_trees.clear();

    int receiver_count = m_view_model->receiverCount();
    int channel_count = m_view_model->channelsPerReceiver();
    if (receiver_count <= 0)
        return;

    // Expand/Collapse All button
    QPushButton* toggle_btn = new QPushButton("Expand All");
    toggle_btn->setFlat(true);
    m_receivers_section_layout->addWidget(toggle_btn, 0, Qt::AlignLeft);

    // Four columns of receiver trees
    const int num_columns = UIConstants::kReceiverGridColumns;
    int actual_columns = qMin(num_columns, receiver_count);
    int per_column = (receiver_count + actual_columns - 1) / actual_columns;

    QHBoxLayout* columns_layout = new QHBoxLayout;
    columns_layout->setSpacing(2);
    columns_layout->setContentsMargins(0, 0, 0, 0);

    for (int col = 0; col < actual_columns; col++)
    {
        int start = col * per_column;
        int end = qMin(start + per_column, receiver_count);
        if (start >= receiver_count)
            break;

        QTreeWidget* tree = new QTreeWidget;
        tree->setHeaderHidden(true);
        tree->setColumnCount(1);
        tree->setRootIsDecorated(true);
        tree->setAnimated(true);
        tree->setIndentation(0);
        tree->setFixedWidth(UIConstants::kTreeFixedWidth);
        tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // Fixed height fits all collapsed receivers; scrollbar appears when expanded
        tree->setFixedHeight(per_column * UIConstants::kTreeItemHeightFactor + UIConstants::kTreeHeightBuffer);

        for (int receiver_index = start; receiver_index < end; receiver_index++)
        {
            QTreeWidgetItem* receiver_item = new QTreeWidgetItem;
            receiver_item->setText(0, "RCVR " + QString::number(receiver_index + 1));
            receiver_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
            receiver_item->setData(0, Qt::UserRole, receiver_index);

            for (int channel_index = 0; channel_index < channel_count; channel_index++)
            {
                QTreeWidgetItem* channel_item = new QTreeWidgetItem;
                channel_item->setText(0, m_view_model->channelPrefix(channel_index));
                channel_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                channel_item->setCheckState(0,
                    m_view_model->receiverChecked(receiver_index, channel_index)
                        ? Qt::Checked : Qt::Unchecked);
                receiver_item->addChild(channel_item);
            }

            tree->addTopLevelItem(receiver_item);
        }

        tree->collapseAll();

        connect(tree, &QTreeWidget::itemChanged,
                this, &MainView::onTreeItemChanged);

        columns_layout->addWidget(tree);
        m_receiver_trees.append(tree);
    }

    columns_layout->addStretch(1);
    m_receivers_section_layout->addLayout(columns_layout);

    // Hide scrollbar on non-last columns; sync all from the visible one
    if (m_receiver_trees.size() > 1)
    {
        QScrollBar* visible_bar = m_receiver_trees.last()->verticalScrollBar();
        for (int i = 0; i < m_receiver_trees.size() - 1; i++)
        {
            m_receiver_trees[i]->verticalScrollBar()->setFixedWidth(0);
            connect(visible_bar, &QScrollBar::valueChanged,
                    m_receiver_trees[i]->verticalScrollBar(), &QScrollBar::setValue);
        }
    }

    // Connect expand/collapse toggle
    connect(toggle_btn, &QPushButton::clicked, this, [this, toggle_btn]() {
        bool any_collapsed = false;
        for (QTreeWidget* t : m_receiver_trees)
            for (int i = 0; i < t->topLevelItemCount(); i++)
                if (!t->topLevelItem(i)->isExpanded())
                    any_collapsed = true;

        for (QTreeWidget* t : m_receiver_trees)
        {
            if (any_collapsed)
                t->expandAll();
            else
                t->collapseAll();
        }
        toggle_btn->setText(any_collapsed ? "Collapse All" : "Expand All");
    });
}

void MainView::setUpTimeSection()
{
    m_time_section = new QGroupBox("Time");
    QGridLayout* time_grid = new QGridLayout;

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
    m_time_section->setLayout(time_grid);
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

    // ViewModel -> View: data binding
    connect(m_view_model, &MainViewModel::inputFilenameChanged, this, [this]() {
        m_input_file->setText(m_view_model->inputFilename());
    });
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
    setAllNumberedReceiversEnabled(loaded);
    m_time_all->setEnabled(loaded);
    m_sample_rate->setEnabled(loaded);
    m_process_action->setEnabled(loaded);

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

        setAllNumberedReceiversEnabled(false);
        setAllNumberedReceiversChecked(true);

        m_time_all->setEnabled(false);
        m_time_all->setChecked(true);
        setAllStartStopTimesEnabled(false);
        clearAllStartStopTimes();
        m_sample_rate->setEnabled(false);
        m_sample_rate->setCurrentIndex(0);

        m_progress_bar->setValue(0);
        m_process_action->setEnabled(false);
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
    }
    else
    {
        setAllControlsEnabled(true);
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

void MainView::onTreeItemChanged(QTreeWidgetItem* item, int column)
{
    if (column != 0)
        return;
    if (m_updating_from_viewmodel)
        return;

    // Only forward leaf (channel) items to the ViewModel.
    // Parent (receiver) items have their state managed by Qt::ItemIsAutoTristate.
    QTreeWidgetItem* parent = item->parent();
    if (!parent)
        return;

    int receiver_index = parent->data(0, Qt::UserRole).toInt();
    int channel_index = parent->indexOfChild(item);
    bool checked = (item->checkState(0) == Qt::Checked);

    m_view_model->setReceiverChecked(receiver_index, channel_index, checked);
}

void MainView::onReceiverCheckedChanged(int receiver_index, int channel_index, bool checked)
{
    // Find the tree item matching this receiver_index via UserRole data
    QTreeWidgetItem* target = nullptr;
    QTreeWidget* target_tree = nullptr;
    for (QTreeWidget* tree : m_receiver_trees)
    {
        for (int i = 0; i < tree->topLevelItemCount(); i++)
        {
            if (tree->topLevelItem(i)->data(0, Qt::UserRole).toInt() == receiver_index)
            {
                target = tree->topLevelItem(i);
                target_tree = tree;
                break;
            }
        }
        if (target)
            break;
    }

    if (!target || channel_index < 0 || channel_index >= target->childCount())
        return;

    m_updating_from_viewmodel = true;
    target_tree->blockSignals(true);
    target->child(channel_index)->setCheckState(
        0, checked ? Qt::Checked : Qt::Unchecked);
    target_tree->blockSignals(false);
    m_updating_from_viewmodel = false;
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
    saveLastDir();
    m_view_model->openFile(filename);
}

void MainView::onSettings()
{
    SettingsDialog dialog(this);
    dialog.setFrameSync(m_view_model->frameSync());
    dialog.setNegativePolarity(m_view_model->negativePolarity());
    dialog.setSlopeIndex(m_view_model->slopeIndex());
    dialog.setScale(m_view_model->scale());
    dialog.setReceiverCount(m_view_model->receiverCount());
    dialog.setChannelsPerReceiver(m_view_model->channelsPerReceiver());

    connect(&dialog, &SettingsDialog::loadRequested, this, [this, &dialog]() {
        QString start_dir = m_view_model->lastSettingsFile().isEmpty()
            ? m_view_model->appRoot() + "/settings"
            : m_view_model->lastSettingsFile();
        QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                        start_dir,
                                                        tr("Configuration Settings Files (*.ini)"));
        if (filename.isEmpty())
            return;

        m_view_model->loadSettings(filename);

        // Update dialog fields from newly loaded config values
        dialog.setFrameSync(m_view_model->frameSync());
        dialog.setNegativePolarity(m_view_model->negativePolarity());
        dialog.setSlopeIndex(m_view_model->slopeIndex());
        dialog.setScale(m_view_model->scale());
        dialog.setReceiverCount(m_view_model->receiverCount());
        dialog.setChannelsPerReceiver(m_view_model->channelsPerReceiver());
    });

    connect(&dialog, &SettingsDialog::saveAsRequested, this, [this, &dialog]() {
        m_view_model->applySettings(dialog.frameSync(), dialog.negativePolarity(),
                                   dialog.slopeIndex(), dialog.scale(),
                                   dialog.receiverCount(), dialog.channelsPerReceiver());

        QString save_dir = m_view_model->lastSettingsFile().isEmpty()
            ? m_view_model->appRoot() + "/settings"
            : QFileInfo(m_view_model->lastSettingsFile()).absolutePath();
        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        save_dir,
                                                        tr("Configuration Settings Files (*.ini)"));
        if (!filename.isEmpty())
            m_view_model->saveSettings(filename);
    });

    if (dialog.exec() == QDialog::Accepted)
    {
        m_view_model->applySettings(dialog.frameSync(), dialog.negativePolarity(),
                                   dialog.slopeIndex(), dialog.scale(),
                                   dialog.receiverCount(), dialog.channelsPerReceiver());
    }
}

void MainView::onToggleTheme()
{
    QSettings app_settings(UIConstants::kOrganizationName, UIConstants::kApplicationName);
    QString current_theme = app_settings.value(UIConstants::kSettingsKeyTheme, UIConstants::kThemeDark).toString();
    QString new_theme = (current_theme == UIConstants::kThemeDark) ? UIConstants::kThemeLight : UIConstants::kThemeDark;

    QString qss_path = (new_theme == UIConstants::kThemeLight)
        ? ":/resources/win11-light.qss"
        : ":/resources/win11-dark.qss";

    QFile qss_file(qss_path);
    if (qss_file.open(QFile::ReadOnly))
    {
        static_cast<QApplication*>(QApplication::instance())->setStyleSheet(
            QLatin1String(qss_file.readAll()));
        qss_file.close();
    }

    app_settings.setValue(UIConstants::kSettingsKeyTheme, new_theme);

    m_theme_action->setText(
        (new_theme == UIConstants::kThemeDark) ? "Switch to Light Theme" : "Switch to Dark Theme");
}

void MainView::timeAllCheckBoxToggled(bool checked)
{
    if (checked)
        fillAllStartStopTimes();

    setAllStartStopTimesEnabled(!checked);
}

void MainView::progressProcessButtonPressed()
{
    // Guard against re-entry
    if (m_view_model->processing())
        return;

    // Validate time fields before prompting for output file
    if (!m_time_all->isChecked())
    {
        QStringList start_parts = m_start_time->text().split(":");
        QStringList stop_parts = m_stop_time->text().split(":");

        if (start_parts.size() != 4 || stop_parts.size() != 4)
        {
            QMessageBox::warning(this, "Invalid Time",
                "Start and stop times must be in DDD:HH:MM:SS format.");
            return;
        }

        bool ok = false;
        int s_ddd = start_parts[0].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Start day is not a valid number."); return; }
        int s_hh  = start_parts[1].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Start hour is not a valid number."); return; }
        int s_mm  = start_parts[2].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Start minute is not a valid number."); return; }
        int s_ss  = start_parts[3].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Start second is not a valid number."); return; }

        int e_ddd = stop_parts[0].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Stop day is not a valid number."); return; }
        int e_hh  = stop_parts[1].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Stop hour is not a valid number."); return; }
        int e_mm  = stop_parts[2].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Stop minute is not a valid number."); return; }
        int e_ss  = stop_parts[3].toInt(&ok); if (!ok) { QMessageBox::warning(this, "Invalid Time", "Stop second is not a valid number."); return; }

        if (s_ddd < UIConstants::kMinDayOfYear || s_ddd > UIConstants::kMaxDayOfYear ||
            s_hh < 0 || s_hh > UIConstants::kMaxHour ||
            s_mm < 0 || s_mm > UIConstants::kMaxMinute ||
            s_ss < 0 || s_ss > UIConstants::kMaxSecond)
        {
            QMessageBox::warning(this, "Invalid Time",
                "Start time is out of range.\n"
                "Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.");
            return;
        }

        if (e_ddd < UIConstants::kMinDayOfYear || e_ddd > UIConstants::kMaxDayOfYear ||
            e_hh < 0 || e_hh > UIConstants::kMaxHour ||
            e_mm < 0 || e_mm > UIConstants::kMaxMinute ||
            e_ss < 0 || e_ss > UIConstants::kMaxSecond)
        {
            QMessageBox::warning(this, "Invalid Time",
                "Stop time is out of range.\n"
                "Day: 1-366, Hour: 0-23, Minute: 0-59, Second: 0-59.");
            return;
        }

        // Compare as composite value: DDD*SecondsPerDay + HH*SecondsPerHour + MM*SecondsPerMinute + SS
        long long start_total = s_ddd * (long long)UIConstants::kSecondsPerDay + s_hh * (long long)UIConstants::kSecondsPerHour + s_mm * (long long)UIConstants::kSecondsPerMinute + s_ss;
        long long stop_total  = e_ddd * (long long)UIConstants::kSecondsPerDay + e_hh * (long long)UIConstants::kSecondsPerHour + e_mm * (long long)UIConstants::kSecondsPerMinute + e_ss;
        if (stop_total <= start_total)
        {
            QMessageBox::warning(this, "Invalid Time",
                "Stop time must be after start time.");
            return;
        }
    }

    QString outfile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    m_last_dir + "/" + m_view_model->generateOutputFilename(),
                                                    tr("CSV Files (*.csv);;All Files (*.*)"));
    if (outfile.isEmpty())
        return;

    m_last_dir = QFileInfo(outfile).absolutePath();
    saveLastDir();

    QStringList start_parts = m_start_time->text().split(":");
    QStringList stop_parts = m_stop_time->text().split(":");

    m_view_model->startProcessing(
        outfile,
        start_parts.value(0), start_parts.value(1),
        start_parts.value(2), start_parts.value(3),
        stop_parts.value(0), stop_parts.value(1),
        stop_parts.value(2), stop_parts.value(3),
        m_sample_rate->currentIndex());
}

////////////////////////////////////////////////////////////////////////////////
//                            DRAG AND DROP                                   //
////////////////////////////////////////////////////////////////////////////////

void MainView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        for (const QUrl& url : event->mimeData()->urls())
        {
            if (url.toLocalFile().endsWith(".ch10", Qt::CaseInsensitive))
            {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainView::dropEvent(QDropEvent* event)
{
    for (const QUrl& url : event->mimeData()->urls())
    {
        QString file = url.toLocalFile();
        if (file.endsWith(".ch10", Qt::CaseInsensitive))
        {
            m_last_dir = QFileInfo(file).absolutePath();
            saveLastDir();
            m_view_model->openFile(file);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//                              HELPER METHODS                                //
////////////////////////////////////////////////////////////////////////////////

void MainView::setAllControlsEnabled(bool enabled)
{
    m_toolbar_open_action->setEnabled(enabled);
    m_time_channel->setEnabled(enabled);
    m_pcm_channel->setEnabled(enabled);
    setAllNumberedReceiversEnabled(enabled);
    m_time_all->setEnabled(enabled);
    m_sample_rate->setEnabled(enabled);
    m_process_action->setEnabled(enabled);

    if (enabled)
    {
        if (!m_time_all->isChecked())
            setAllStartStopTimesEnabled(true);
    }
    else
    {
        setAllStartStopTimesEnabled(false);
    }
}

void MainView::setAllNumberedReceiversEnabled(bool enabled)
{
    for (QTreeWidget* tree : m_receiver_trees)
        tree->setEnabled(enabled);
}

void MainView::setAllNumberedReceiversChecked(bool checked)
{
    Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;
    for (QTreeWidget* tree : m_receiver_trees)
    {
        tree->blockSignals(true);
        for (int r = 0; r < tree->topLevelItemCount(); r++)
        {
            QTreeWidgetItem* receiver_item = tree->topLevelItem(r);
            for (int c = 0; c < receiver_item->childCount(); c++)
                receiver_item->child(c)->setCheckState(0, state);
        }
        tree->blockSignals(false);
    }
}

void MainView::setAllStartStopTimesEnabled(bool enabled)
{
    m_start_time->setEnabled(enabled);
    m_stop_time->setEnabled(enabled);
}

void MainView::fillAllStartStopTimes()
{
    m_start_time->setText(
        QString("%1:%2:%3:%4")
            .arg(m_view_model->startDayOfYear(), 3, 10, QChar('0'))
            .arg(m_view_model->startHour(), 2, 10, QChar('0'))
            .arg(m_view_model->startMinute(), 2, 10, QChar('0'))
            .arg(m_view_model->startSecond(), 2, 10, QChar('0')));

    m_stop_time->setText(
        QString("%1:%2:%3:%4")
            .arg(m_view_model->stopDayOfYear(), 3, 10, QChar('0'))
            .arg(m_view_model->stopHour(), 2, 10, QChar('0'))
            .arg(m_view_model->stopMinute(), 2, 10, QChar('0'))
            .arg(m_view_model->stopSecond(), 2, 10, QChar('0')));
}

void MainView::clearAllStartStopTimes()
{
    m_start_time->clear();
    m_stop_time->clear();
}
