/**
 * @file mainview.cpp
 * @brief Implementation of MainView — Qt Widgets UI and ViewModel signal wiring.
 */

#include "mainview.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QSignalBlocker>
#include <QTime>
#include <QUrl>

#include "constants.h"
#include "mainviewmodel.h"
#include "receivergridwidget.h"
#include "settingsdialog.h"
#include "timeextractionwidget.h"


MainView::MainView(QWidget *parent)
    : QMainWindow(parent)
{
    m_view_model = new MainViewModel(this);

    setAcceptDrops(true);

    setUpMainLayout();

    QSettings app_settings;
    m_last_ch10_dir = app_settings.value(UIConstants::kSettingsKeyLastCh10Dir).toString();
    m_last_csv_dir  = app_settings.value(UIConstants::kSettingsKeyLastCsvDir).toString();

    setUpConnections();
    m_view_model->logStartupInfo();
}

MainView::~MainView()
{
}

void MainView::saveLastCh10Dir()
{
    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyLastCh10Dir, m_last_ch10_dir);
}

void MainView::saveLastCsvDir()
{
    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyLastCsvDir, m_last_csv_dir);
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

    m_receiver_grid = new ReceiverGridWidget;
    m_receiver_grid->rebuild(
        m_view_model->receiverCount(),
        m_view_model->channelsPerReceiver(),
        [this](int i) { return m_view_model->channelPrefix(i); },
        [this](int r, int c) { return m_view_model->receiverChecked(r, c); });

    m_time_widget = new TimeExtractionWidget;

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
    m_controls_layout->addWidget(m_receiver_grid);
    m_controls_layout->addWidget(m_time_widget);
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

    m_controls_dock = new QDockWidget(this);
    m_controls_dock->setTitleBarWidget(new QWidget);
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

    m_receiver_grid->setAllEnabled(false);
    m_receiver_grid->setAllChecked(true);

    m_time_widget->setAllEnabled(false);
    m_time_widget->setExtractAllTime(true);
    m_time_widget->clearTimes();
    m_time_widget->setSampleRateIndex(0);

    m_progress_bar->setValue(0);

    resize(minimumSizeHint());
}

void MainView::setUpMenuBar()
{
    QMenuBar* menu_bar = menuBar();
    QMenu* file_menu = menu_bar->addMenu("&File");

    QAction* settings_action = file_menu->addAction("Settings...");
    file_menu->addSeparator();

    QSettings app_settings;
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

    // TimeExtractionWidget -> ViewModel
    connect(m_time_widget, &TimeExtractionWidget::extractAllTimeChanged,
            m_view_model, &MainViewModel::setExtractAllTime);
    connect(m_time_widget, &TimeExtractionWidget::sampleRateIndexChanged,
            m_view_model, &MainViewModel::setSampleRateIndex);

    // ReceiverGridWidget -> ViewModel
    connect(m_receiver_grid, &ReceiverGridWidget::receiverChecked,
            m_view_model, &MainViewModel::setReceiverChecked);

    // ViewModel -> View: data binding
    connect(m_view_model, &MainViewModel::inputFilenameChanged, this, [this]() {
        m_input_file->setText(m_view_model->inputFilename());
    });
    connect(m_view_model, &MainViewModel::channelListsChanged, this, &MainView::onChannelListsChanged);
    connect(m_view_model, &MainViewModel::fileLoadedChanged, this, &MainView::onFileLoadedChanged);
    connect(m_view_model, &MainViewModel::fileTimesChanged, this, &MainView::onFileTimesChanged);
    connect(m_view_model, &MainViewModel::progressPercentChanged, this, &MainView::onProgressChanged);
    connect(m_view_model, &MainViewModel::processingChanged, this, &MainView::onProcessingChanged);

    // ViewModel -> ReceiverGridWidget
    connect(m_view_model, &MainViewModel::receiverLayoutChanged, this, [this]() {
        m_receiver_grid->rebuild(
            m_view_model->receiverCount(),
            m_view_model->channelsPerReceiver(),
            [this](int i) { return m_view_model->channelPrefix(i); },
            [this](int r, int c) { return m_view_model->receiverChecked(r, c); });
    });
    connect(m_view_model, &MainViewModel::receiverCheckedChanged,
            m_receiver_grid, &ReceiverGridWidget::setReceiverChecked);

    // ViewModel -> TimeExtractionWidget
    connect(m_view_model, &MainViewModel::extractAllTimeChanged, this, [this]() {
        m_time_widget->setExtractAllTime(m_view_model->extractAllTime());
    });
    connect(m_view_model, &MainViewModel::sampleRateIndexChanged, this, [this]() {
        m_time_widget->setSampleRateIndex(m_view_model->sampleRateIndex());
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
}

void MainView::onFileLoadedChanged()
{
    bool loaded = m_view_model->fileLoaded();

    m_time_channel->setEnabled(loaded);
    m_pcm_channel->setEnabled(loaded);
    m_receiver_grid->setAllEnabled(loaded);
    m_time_widget->setAllEnabled(loaded);
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

        m_receiver_grid->setAllEnabled(false);
        m_receiver_grid->setAllChecked(true);

        m_time_widget->setAllEnabled(false);
        m_time_widget->setExtractAllTime(true);
        m_time_widget->clearTimes();
        m_time_widget->setSampleRateIndex(0);

        m_progress_bar->setValue(0);
        m_process_action->setEnabled(false);
    }
}

void MainView::onFileTimesChanged()
{
    if (!m_view_model->fileLoaded())
        return;
    m_time_widget->fillTimes(
        m_view_model->startDayOfYear(), m_view_model->startHour(),
        m_view_model->startMinute(), m_view_model->startSecond(),
        m_view_model->stopDayOfYear(), m_view_model->stopHour(),
        m_view_model->stopMinute(), m_view_model->stopSecond());
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
        m_process_action->setEnabled(true);
        m_process_action->setIcon(QIcon(":/resources/stop.svg"));
        m_process_action->setToolTip("Cancel Processing");
        m_progress_bar->setValue(0);
    }
    else
    {
        setAllControlsEnabled(true);
        m_process_action->setIcon(QIcon(":/resources/play.svg"));
        m_process_action->setToolTip("Process Ch10 to CSV");
    }
}

void MainView::onProcessingFinished(bool success, const QString& output_file)
{
    if (success)
    {
        m_progress_bar->setValue(100);

        QMessageBox msg_box(this);
        msg_box.setWindowTitle("HUZZAH!");
        msg_box.setText("Processing complete!\nFile saved to:\n" + output_file);
        msg_box.setIcon(QMessageBox::Information);

        QPushButton* open_file_btn = msg_box.addButton("Open File", QMessageBox::ActionRole);
        QPushButton* open_folder_btn = msg_box.addButton("Open Folder", QMessageBox::ActionRole);
        msg_box.addButton("Close", QMessageBox::RejectRole);

        msg_box.exec();

        if (msg_box.clickedButton() == open_file_btn)
            QDesktopServices::openUrl(QUrl::fromLocalFile(output_file));
        else if (msg_box.clickedButton() == open_folder_btn)
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(output_file).absolutePath()));
    }
}

void MainView::onLogMessage(const QString& message)
{
    if (message.contains("ERROR"))
        logError(message);
    else if (message.contains("WARNING"))
        logWarning(message);
    else if (message.startsWith("Pre-scan result:") || message.startsWith("Processing complete"))
        logSuccess(message);
    else
        m_log_window->appendPlainText(
            QTime::currentTime().toString("HH:mm:ss") + "  " + message);
    m_log_window->verticalScrollBar()->setValue(m_log_window->verticalScrollBar()->maximum());
}

////////////////////////////////////////////////////////////////////////////////
//                         USER-INITIATED ACTIONS                             //
////////////////////////////////////////////////////////////////////////////////

void MainView::displayErrorMessage(const QString& message)
{
    logError(message);
}

void MainView::inputFileButtonPressed()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    m_last_ch10_dir,
                                                    tr("Chapter 10 Files (*.ch10);;All Files (*.*)"));

    if (filename.isEmpty())
        return;

    m_last_ch10_dir = QFileInfo(filename).absolutePath();
    saveLastCh10Dir();
    m_view_model->openFile(filename);
}

void MainView::onSettings()
{
    SettingsDialog dialog(this);
    dialog.setData(m_view_model->getSettingsData());

    connect(&dialog, &SettingsDialog::loadRequested, this, [this, &dialog]() {
        QString start_dir = m_view_model->lastIniDir();
        QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                        start_dir,
                                                        tr("Configuration Settings Files (*.ini)"));
        if (filename.isEmpty())
            return;

        m_view_model->loadSettings(filename);
        dialog.setData(m_view_model->getSettingsData());
    });

    connect(&dialog, &SettingsDialog::saveAsRequested, this, [this, &dialog]() {
        m_view_model->applySettingsData(dialog.getData());

        QString save_dir = m_view_model->lastIniDir();
        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        save_dir,
                                                        tr("Configuration Settings Files (*.ini)"));
        if (!filename.isEmpty())
            m_view_model->saveSettings(filename);
    });

    if (dialog.exec() == QDialog::Accepted)
        m_view_model->applySettingsData(dialog.getData());
}

void MainView::onToggleTheme()
{
    QSettings app_settings;
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

void MainView::progressProcessButtonPressed()
{
    if (m_view_model->processing())
    {
        m_view_model->cancelProcessing();
        return;
    }

    // Pre-validate time fields before prompting for output file
    if (!m_time_widget->extractAllTime())
    {
        QString warning = m_view_model->validateTimeRange(
            m_time_widget->startTimeText(), m_time_widget->stopTimeText());
        if (!warning.isEmpty())
        {
            logWarning(warning);
            return;
        }
    }

    QString outfile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    m_last_csv_dir + "/" + m_view_model->generateOutputFilename(),
                                                    tr("CSV Files (*.csv);;All Files (*.*)"));
    if (outfile.isEmpty())
        return;

    m_last_csv_dir = QFileInfo(outfile).absolutePath();
    saveLastCsvDir();

    QStringList start_parts = m_time_widget->startTimeText().split(":");
    QStringList stop_parts = m_time_widget->stopTimeText().split(":");

    m_view_model->startProcessing(
        outfile,
        start_parts.value(0), start_parts.value(1),
        start_parts.value(2), start_parts.value(3),
        stop_parts.value(0), stop_parts.value(1),
        stop_parts.value(2), stop_parts.value(3),
        m_time_widget->sampleRateIndex());
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
            m_last_ch10_dir = QFileInfo(file).absolutePath();
            saveLastCh10Dir();
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
    m_receiver_grid->setAllEnabled(enabled);
    m_time_widget->setAllEnabled(enabled);
    m_process_action->setEnabled(enabled);
}

void MainView::logError(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    m_log_window->appendHtml(
        "<span style='color: red;'>" + timestamp + "  " +
        message.toHtmlEscaped() + "</span>");
}

void MainView::logWarning(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    m_log_window->appendHtml(
        "<span style='color: #DAA520;'>" + timestamp + "  " +
        message.toHtmlEscaped() + "</span>");
}

void MainView::logSuccess(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    m_log_window->appendHtml(
        "<span style='color: green;'>" + timestamp + "  " +
        message.toHtmlEscaped() + "</span>");
}
