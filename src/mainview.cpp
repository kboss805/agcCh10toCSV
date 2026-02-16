/**
 * @file mainview.cpp
 * @brief Implementation of MainView — Qt Widgets UI and ViewModel signal wiring.
 */

#include "mainview.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QFrame>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTime>
#include <QUrl>

#include "constants.h"
#include "mainviewmodel.h"
#include "plotviewmodel.h"
#include "plotwidget.h"
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
    if (m_last_ch10_dir.isEmpty())
        m_last_ch10_dir = QCoreApplication::applicationDirPath();
    m_last_csv_dir  = app_settings.value(UIConstants::kSettingsKeyLastCsvDir).toString();
    m_last_batch_output_dir = app_settings.value(UIConstants::kSettingsKeyLastBatchDir).toString();

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
    m_controls_layout->setSpacing(0);
    m_controls_layout->setContentsMargins(2, 8, 16, 8);

    // set up constituent parts
    setUpMenuBar();

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

    m_settings_tree = new QTreeWidget;
    m_settings_tree->setHeaderHidden(true);
    m_settings_tree->setColumnCount(2);
    m_settings_tree->setRootIsDecorated(true);
    m_settings_tree->setIndentation(12);
    m_settings_tree->setSelectionMode(QAbstractItemView::NoSelection);
    m_settings_tree->setFocusPolicy(Qt::NoFocus);
    m_settings_tree->setAnimated(true);
    m_settings_tree->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_settings_tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Dynamic height: collapsed = 1 row, expanded = root + children
    int row_h = m_settings_tree->fontMetrics().height() + 8;
    int frame_h = m_settings_tree->frameWidth() * 2;
    int collapsed_h = row_h + frame_h;
    m_settings_tree->setFixedHeight(collapsed_h);

    connect(m_settings_tree, &QTreeWidget::itemExpanded, this, [this, row_h, frame_h](QTreeWidgetItem* item) {
        int total_rows = 1 + item->childCount();
        m_settings_tree->setFixedHeight(total_rows * row_h + frame_h);
    });
    connect(m_settings_tree, &QTreeWidget::itemCollapsed, this, [this, collapsed_h]() {
        m_settings_tree->setFixedHeight(collapsed_h);
    });

    setUpFileList();

    m_log_preview = new QTextBrowser;
    m_log_preview->setReadOnly(true);
    m_log_preview->setOpenLinks(false);
    m_log_preview->setMinimumHeight(UIConstants::kLogPreviewHeight);

    m_controls_layout->addWidget(m_settings_tree);
    m_controls_layout->addSpacing(8);
    m_controls_layout->addWidget(m_file_list);
    m_controls_layout->addSpacing(8);
    m_controls_layout->addWidget(m_receiver_grid);
    m_controls_layout->addSpacing(8);
    m_controls_layout->addWidget(m_time_widget);
    m_controls_layout->addSpacing(8);
    m_controls_layout->addWidget(m_log_preview, 1);
    m_controls_layout->addSpacing(8);
    m_controls_layout->addWidget(m_progress_bar);

    // PlotWidget as central widget — fills all space right of the controls dock
    m_plot_view_model = new PlotViewModel(this);
    m_plot_widget = new PlotWidget;
    m_plot_widget->setViewModel(m_plot_view_model);
    m_plot_widget->setMinimumSize(PlotConstants::kPlotDockMinWidth, PlotConstants::kPlotDockMinHeight);

    QSettings plot_settings;
    bool dark = plot_settings.value(UIConstants::kSettingsKeyTheme, UIConstants::kThemeDark).toString()
                == UIConstants::kThemeDark;
    m_plot_widget->applyTheme(dark);
    m_plot_widget->initReceiverLegend(
        m_view_model->receiverCount(),
        m_view_model->channelsPerReceiver(),
        [this](int i) { return m_view_model->channelPrefix(i); });

    // Wrap plot in a layout with a vertical separator on the left
    QWidget* central_wrapper = new QWidget;
    QHBoxLayout* central_layout = new QHBoxLayout(central_wrapper);
    central_layout->setContentsMargins(0, 0, 0, 0);
    central_layout->setSpacing(16);

    QFrame* vsep = new QFrame;
    vsep->setFrameShape(QFrame::VLine);
    vsep->setFrameShadow(QFrame::Sunken);
    central_layout->addWidget(vsep);
    central_layout->addWidget(m_plot_widget, 1);

    setCentralWidget(central_wrapper);

    // Log as standalone dialog window
    m_log_dialog = new QDialog(this);
    m_log_dialog->setWindowTitle("Log");
    m_log_dialog->resize(600, 400);
    m_log_window = new QTextBrowser;
    m_log_window->setReadOnly(true);
    m_log_window->setOpenLinks(false);
    m_log_window->setMinimumWidth(UIConstants::kLogMinimumWidth);
    QVBoxLayout* log_layout = new QVBoxLayout(m_log_dialog);
    log_layout->setContentsMargins(0, 0, 0, 0);
    log_layout->addWidget(m_log_window);

    // Controls in a fixed left dock widget
    QWidget* controls_widget = new QWidget;
    controls_widget->setLayout(m_controls_layout);
    controls_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_controls_dock = new QDockWidget(this);
    m_controls_dock->setTitleBarWidget(new QWidget);
    m_controls_dock->setWidget(controls_widget);
    m_controls_dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::LeftDockWidgetArea, m_controls_dock);

    // additional settings
    setWindowTitle("Chapter 10 to CSV AGC Converter");

    // Initialize UI to disabled state
    m_receiver_grid->setAllEnabled(false);
    m_receiver_grid->setAllChecked(true);

    m_time_widget->setAllEnabled(false);
    m_time_widget->setExtractAllTime(true);
    m_time_widget->clearTimes();
    m_time_widget->setSampleRateIndex(UIConstants::kDefaultSampleRateIndex);

    m_progress_bar->setValue(0);

    statusBar()->showMessage("No file loaded");
    updateSettingsSummary();

    showMaximized();
}

void MainView::setUpMenuBar()
{
    QMenuBar* menu_bar = menuBar();
    QMenu* file_menu = menu_bar->addMenu("&File");

    QAction* settings_action = file_menu->addAction("Settings...");
    m_recent_menu = file_menu->addMenu("Recent Files");
    updateRecentFilesMenu();
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

    QMenu* view_menu = menu_bar->addMenu("&View");
    m_show_log_action = view_menu->addAction("Show Log");
    connect(m_show_log_action, &QAction::triggered, this, [this]() {
        m_toggle_log_action->setChecked(true);
    });

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

    m_cancel_action = m_toolbar->addAction(
        QIcon(":/resources/stop.svg"), "Cancel");
    m_cancel_action->setToolTip("Cancel Processing");
    m_cancel_action->setEnabled(false);
    connect(m_cancel_action, &QAction::triggered,
            this, [this]() { m_view_model->cancelProcessing(); });

    QWidget* toolbar_spacer = new QWidget;
    toolbar_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(toolbar_spacer);

    m_toggle_log_action = m_toolbar->addAction(
        QIcon(":/resources/magnifying-glass.svg"), "Toggle Log");
    m_toggle_log_action->setToolTip("Show/Hide Log Window");
    m_toggle_log_action->setCheckable(true);
    m_toggle_log_action->setChecked(false);
    connect(m_toggle_log_action, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_log_dialog->show();
            m_log_dialog->raise();
        } else {
            m_log_dialog->hide();
        }
    });
}


void MainView::setUpFileList()
{
    m_file_list = new QTreeWidget;
    m_file_list->setHeaderHidden(true);
    m_file_list->setColumnCount(3);
    m_file_list->setRootIsDecorated(true);
    m_file_list->setAnimated(true);
    m_file_list->setIndentation(12);
    m_file_list->setFixedHeight(UIConstants::kBatchFileListHeight);
    m_file_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_file_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_file_list->setSelectionMode(QAbstractItemView::NoSelection);

    QTreeWidgetItem* item = new QTreeWidgetItem;
    item->setText(0, "No file loaded");
    item->setFlags(Qt::ItemIsEnabled);
    m_file_list->addTopLevelItem(item);
    m_file_list->header()->setStretchLastSection(false);
    m_file_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void MainView::onShowPlot(const QString& csv_filepath)
{
    m_plot_view_model->loadCsvFile(csv_filepath);
}

////////////////////////////////////////////////////////////////////////////////
//                              GUI CONNECTIONS                               //
////////////////////////////////////////////////////////////////////////////////

void MainView::setUpConnections()
{
    // TimeExtractionWidget -> ViewModel
    connect(m_time_widget, &TimeExtractionWidget::extractAllTimeChanged,
            m_view_model, &MainViewModel::setExtractAllTime);
    connect(m_time_widget, &TimeExtractionWidget::sampleRateIndexChanged,
            m_view_model, &MainViewModel::setSampleRateIndex);

    // ReceiverGridWidget -> ViewModel
    connect(m_receiver_grid, &ReceiverGridWidget::receiverChecked,
            m_view_model, &MainViewModel::setReceiverChecked);
    connect(m_receiver_grid, &ReceiverGridWidget::selectAllRequested, this, [this]() {
        m_view_model->setAllReceiversChecked(true);
        m_receiver_grid->setAllChecked(true);
    });
    connect(m_receiver_grid, &ReceiverGridWidget::selectNoneRequested, this, [this]() {
        m_view_model->setAllReceiversChecked(false);
        m_receiver_grid->setAllChecked(false);
    });

    // ViewModel -> View: data binding
    connect(m_view_model, &MainViewModel::inputFilenameChanged, this, &MainView::updateFileList);
    connect(m_view_model, &MainViewModel::batchFilesChanged, this, &MainView::updateFileList);
    connect(m_view_model, &MainViewModel::batchModeChanged, this, &MainView::updateFileList);
    connect(m_view_model, &MainViewModel::batchFileUpdated, this, [this](int fileIndex) {
        // Update just the status cell for the affected file — no full tree rebuild
        if (m_file_list->topLevelItemCount() == 0)
            return;
        QTreeWidgetItem* root = m_file_list->topLevelItem(0);
        if (fileIndex < 0 || fileIndex >= root->childCount())
            return;
        QTreeWidgetItem* file_item = root->child(fileIndex);
        const QVector<BatchFileInfo>& files = m_view_model->batchFiles();
        const BatchFileInfo& info = files[fileIndex];

        if (info.skip)
        {
            file_item->setText(1, "Skip");
            file_item->setForeground(1, QColor("#DAA520"));
            file_item->setToolTip(1, info.skipReason);
        }
        else
        {
            file_item->setText(1, "Ready");
            file_item->setForeground(1, QColor("green"));
            file_item->setToolTip(1, QString());
        }

        // Update summary root text
        root->setText(0, m_view_model->batchStatusSummary());
    });
    connect(m_view_model, &MainViewModel::batchFileProcessing, this, [this](int index, int total) {
        statusBar()->showMessage("Processing file " + QString::number(index + 1) +
                                " of " + QString::number(total));
    });
    connect(m_view_model, &MainViewModel::channelListsChanged, this, &MainView::onChannelListsChanged);
    connect(m_view_model, &MainViewModel::fileLoadedChanged, this, &MainView::onFileLoadedChanged);
    connect(m_view_model, &MainViewModel::fileLoadedChanged, this, &MainView::updateStatusBar);
    connect(m_view_model, &MainViewModel::settingsChanged, this, &MainView::updateSettingsSummary);
    connect(m_view_model, &MainViewModel::receiverLayoutChanged, this, &MainView::updateSettingsSummary);
    connect(m_view_model, &MainViewModel::recentFilesChanged, this, &MainView::updateRecentFilesMenu);
    connect(m_view_model, &MainViewModel::fileTimesChanged, this, &MainView::onFileTimesChanged);
    connect(m_view_model, &MainViewModel::progressPercentChanged, this, &MainView::onProgressChanged);
    connect(m_view_model, &MainViewModel::processingChanged, this, &MainView::onProcessingChanged);

    // ViewModel -> ReceiverGridWidget + PlotWidget legend
    connect(m_view_model, &MainViewModel::receiverLayoutChanged, this, [this]() {
        m_receiver_grid->rebuild(
            m_view_model->receiverCount(),
            m_view_model->channelsPerReceiver(),
            [this](int i) { return m_view_model->channelPrefix(i); },
            [this](int r, int c) { return m_view_model->receiverChecked(r, c); });
        m_plot_widget->initReceiverLegend(
            m_view_model->receiverCount(),
            m_view_model->channelsPerReceiver(),
            [this](int i) { return m_view_model->channelPrefix(i); });
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

    connect(m_log_window, &QTextBrowser::anchorClicked, this, [](const QUrl& url) {
        QDesktopServices::openUrl(url);
    });

    // Uncheck toolbar toggle when user closes the log dialog via its own X button
    connect(m_log_dialog, &QDialog::finished, this, [this]() {
        m_toggle_log_action->setChecked(false);
    });
}

////////////////////////////////////////////////////////////////////////////////
//                        VIEWMODEL-DRIVEN SLOTS                              //
////////////////////////////////////////////////////////////////////////////////

void MainView::onChannelListsChanged()
{
    updateFileList();
}

void MainView::onFileLoadedChanged()
{
    bool loaded = m_view_model->fileLoaded();

    m_receiver_grid->setAllEnabled(loaded);
    m_process_action->setEnabled(loaded);

    if (loaded)
    {
        if (m_view_model->batchMode())
        {
            // Batch mode: extract-all forced, time fields disabled, sample rate stays active
            m_time_widget->setExtractAllTime(true);
            m_view_model->setExtractAllTime(true);
            m_time_widget->setAllEnabled(false);
            m_time_widget->setSampleRateEnabled(true);
        }
        else
        {
            // Single file: enable time controls, auto-select first channels
            m_time_widget->setAllEnabled(true);
            if (m_view_model->timeChannelIndex() == 0)
                m_view_model->setTimeChannelIndex(1);
            if (m_view_model->pcmChannelIndex() == 0)
                m_view_model->setPcmChannelIndex(1);
        }
    }
    else
    {
        m_receiver_grid->setAllEnabled(false);
        m_receiver_grid->setAllChecked(true);

        m_time_widget->setAllEnabled(false);
        m_time_widget->setExtractAllTime(true);
        m_time_widget->clearTimes();
        m_time_widget->setSampleRateIndex(UIConstants::kDefaultSampleRateIndex);

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
        m_process_action->setEnabled(false);
        m_cancel_action->setEnabled(true);
        m_progress_bar->setValue(0);
    }
    else
    {
        setAllControlsEnabled(true);
        m_cancel_action->setEnabled(false);
    }
}

void MainView::onProcessingFinished(bool success, const QString& output_file)
{
    if (m_view_model->batchMode())
    {
        if (success)
        {
            m_progress_bar->setValue(100);
            QString folder_url = QUrl::fromLocalFile(output_file).toString();
            QString ts = QTime::currentTime().toString("HH:mm:ss");
            QString html = "<span style='color: green;'>" + ts +
                           "  Batch complete. [<a href='" + folder_url + "'>Open Output Folder</a>]</span>";
            m_log_window->append(html);
            m_log_window->verticalScrollBar()->setValue(m_log_window->verticalScrollBar()->maximum());
            m_log_preview->append(html);
            m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());

            // Collect successfully processed files for plot selection
            QStringList csv_paths;
            QStringList display_names;
            const QVector<BatchFileInfo>& files = m_view_model->batchFiles();
            for (const BatchFileInfo& info : files)
            {
                if (info.processedOk && !info.outputFile.isEmpty())
                {
                    csv_paths.append(info.outputFile);
                    display_names.append(QFileInfo(info.outputFile).fileName());
                }
            }

            if (!csv_paths.isEmpty())
            {
                QDialog plot_dialog(this);
                plot_dialog.setWindowTitle("View AGC Plot");
                QVBoxLayout* layout = new QVBoxLayout(&plot_dialog);

                layout->addWidget(new QLabel("Select a recording to plot:"));

                QComboBox* combo = new QComboBox;
                combo->addItems(display_names);
                layout->addWidget(combo);

                QHBoxLayout* btn_layout = new QHBoxLayout;
                btn_layout->addStretch();
                QPushButton* cancel_btn = new QPushButton("Cancel");
                QPushButton* ok_btn = new QPushButton("OK");
                ok_btn->setDefault(true);
                btn_layout->addWidget(cancel_btn);
                btn_layout->addWidget(ok_btn);
                layout->addLayout(btn_layout);

                connect(cancel_btn, &QPushButton::clicked, &plot_dialog, &QDialog::reject);
                connect(ok_btn, &QPushButton::clicked, &plot_dialog, &QDialog::accept);

                if (plot_dialog.exec() == QDialog::Accepted)
                    onShowPlot(csv_paths[combo->currentIndex()]);
            }
        }
        updateFileList();
    }
    else
    {
        if (success)
        {
            m_progress_bar->setValue(100);

            QString file_url = QUrl::fromLocalFile(output_file).toString();
            QString folder_url = QUrl::fromLocalFile(QFileInfo(output_file).absolutePath()).toString();
            QString ts = QTime::currentTime().toString("HH:mm:ss");
            QString html = "<span style='color: green;'>" + ts +
                           "  Output: <a href='" + file_url + "'>" + output_file.toHtmlEscaped() + "</a>"
                           " [<a href='" + folder_url + "'>Open Folder</a>]</span>";
            m_log_window->append(html);
            m_log_window->verticalScrollBar()->setValue(m_log_window->verticalScrollBar()->maximum());
            m_log_preview->append(html);
            m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());

            onShowPlot(output_file);
        }
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
    {
        QString plain = QTime::currentTime().toString("HH:mm:ss") + "  " + message.toHtmlEscaped();
        m_log_window->append(plain);
        m_log_preview->append(plain);
        m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());
    }
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
    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Open Ch10 Files"),
                                                          m_last_ch10_dir,
                                                          tr("Chapter 10 Files (*.ch10);;All Files (*.*)"));

    if (filenames.isEmpty())
        return;

    m_last_ch10_dir = QFileInfo(filenames.first()).absolutePath();
    saveLastCh10Dir();

    if (filenames.size() == 1)
        m_view_model->openFile(filenames.first());
    else
        m_view_model->openFiles(filenames);
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

    m_plot_widget->applyTheme(new_theme == UIConstants::kThemeDark);
}

void MainView::progressProcessButtonPressed()
{
    if (m_view_model->processing())
    {
        m_view_model->cancelProcessing();
        return;
    }

    // Batch mode: always extract-all, prompt for output directory only
    if (m_view_model->batchMode())
    {
        QString out_dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                             m_last_batch_output_dir);
        if (out_dir.isEmpty())
            return;

        m_last_batch_output_dir = out_dir;
        saveLastBatchOutputDir();

        m_view_model->startBatchProcessing(out_dir, m_time_widget->sampleRateIndex());
        return;
    }

    // Single-file mode: pre-validate time fields before prompting for output file
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

    QStringList start_parts = m_time_widget->startTimeText().split(":");
    QStringList stop_parts = m_time_widget->stopTimeText().split(":");

    {
        QString outfile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        m_last_csv_dir + "/" + m_view_model->generateOutputFilename(),
                                                        tr("CSV Files (*.csv);;All Files (*.*)"));
        if (outfile.isEmpty())
            return;

        m_last_csv_dir = QFileInfo(outfile).absolutePath();
        saveLastCsvDir();

        m_view_model->startProcessing(
            outfile,
            start_parts.value(0), start_parts.value(1),
            start_parts.value(2), start_parts.value(3),
            stop_parts.value(0), stop_parts.value(1),
            stop_parts.value(2), stop_parts.value(3),
            m_time_widget->sampleRateIndex());
    }
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
    QStringList ch10_files;
    for (const QUrl& url : event->mimeData()->urls())
    {
        QString file = url.toLocalFile();
        if (file.endsWith(".ch10", Qt::CaseInsensitive))
            ch10_files.append(file);
    }

    if (ch10_files.isEmpty())
        return;

    m_last_ch10_dir = QFileInfo(ch10_files.first()).absolutePath();
    saveLastCh10Dir();

    if (ch10_files.size() == 1)
        m_view_model->openFile(ch10_files.first());
    else
        m_view_model->openFiles(ch10_files);
}

////////////////////////////////////////////////////////////////////////////////
//                              HELPER METHODS                                //
////////////////////////////////////////////////////////////////////////////////

void MainView::setAllControlsEnabled(bool enabled)
{
    m_toolbar_open_action->setEnabled(enabled);
    m_receiver_grid->setAllEnabled(enabled);
    m_process_action->setEnabled(enabled);
    m_file_list->setEnabled(enabled);

    if (m_view_model->batchMode())
    {
        m_time_widget->setAllEnabled(false);
        m_time_widget->setSampleRateEnabled(enabled);
    }
    else
    {
        m_time_widget->setAllEnabled(enabled);
    }
}

void MainView::logError(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    QString html = "<span style='color: red;'>" + timestamp + "  " +
                   message.toHtmlEscaped() + "</span>";
    m_log_window->append(html);
    m_log_preview->append(html);
    m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());
}

void MainView::logWarning(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    QString html = "<span style='color: #DAA520;'>" + timestamp + "  " +
                   message.toHtmlEscaped() + "</span>";
    m_log_window->append(html);
    m_log_preview->append(html);
    m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());
}

void MainView::logSuccess(const QString& message)
{
    QString timestamp = QTime::currentTime().toString("HH:mm:ss");
    QString html = "<span style='color: green;'>" + timestamp + "  " +
                   message.toHtmlEscaped() + "</span>";
    m_log_window->append(html);
    m_log_preview->append(html);
    m_log_preview->verticalScrollBar()->setValue(m_log_preview->verticalScrollBar()->maximum());
}

void MainView::updateStatusBar()
{
    statusBar()->showMessage(m_view_model->fileMetadataSummary());
}

void MainView::updateRecentFilesMenu()
{
    m_recent_menu->clear();
    QStringList recent = m_view_model->recentFiles();

    if (recent.isEmpty())
    {
        QAction* placeholder = m_recent_menu->addAction("(No recent files)");
        placeholder->setEnabled(false);
        return;
    }

    for (const QString& filepath : recent)
    {
        QString display = QFileInfo(filepath).fileName();
        QAction* action = m_recent_menu->addAction(display);
        action->setToolTip(filepath);
        connect(action, &QAction::triggered, this, [this, filepath]() {
            m_last_ch10_dir = QFileInfo(filepath).absolutePath();
            saveLastCh10Dir();
            m_view_model->openFile(filepath);
        });
    }

    m_recent_menu->addSeparator();
    QAction* clear_action = m_recent_menu->addAction("Clear Recent Files");
    connect(clear_action, &QAction::triggered, this, [this]() {
        m_view_model->clearRecentFiles();
    });
}

void MainView::updateSettingsSummary()
{
    bool was_expanded = false;
    if (m_settings_tree->topLevelItemCount() > 0)
        was_expanded = m_settings_tree->topLevelItem(0)->isExpanded();

    m_settings_tree->clear();

    QTreeWidgetItem* root = new QTreeWidgetItem;
    root->setText(0, "Settings");
    root->setFlags(Qt::ItemIsEnabled);

    auto addRow = [&](const QString& label, const QString& value) {
        QTreeWidgetItem* item = new QTreeWidgetItem;
        item->setText(0, label);
        item->setText(1, value);
        item->setFlags(Qt::ItemIsEnabled);
        root->addChild(item);
    };

    addRow("Sync", m_view_model->frameSync());
    addRow("Polarity", QString(UIConstants::kPolarityLabels[m_view_model->polarityIndex()]));
    addRow("Slope", QString(UIConstants::kSlopeLabels[m_view_model->slopeIndex()]));
    addRow("Scale", m_view_model->scale() + " dB/V");
    addRow("Receivers", QString::number(m_view_model->receiverCount()) +
           " x " + QString::number(m_view_model->channelsPerReceiver()) + " ch");

    m_settings_tree->addTopLevelItem(root);
    root->setExpanded(was_expanded);

    m_settings_tree->resizeColumnToContents(0);
}

void MainView::updateFileList()
{
    m_file_list->clear();

    if (m_view_model->batchMode())
    {
        m_file_list->setHeaderHidden(true);
        m_file_list->setFixedHeight(UIConstants::kBatchFileListHeight);

        const QVector<BatchFileInfo>& files = m_view_model->batchFiles();

        QTreeWidgetItem* root = new QTreeWidgetItem;
        root->setText(0, m_view_model->batchStatusSummary());
        root->setFlags(Qt::ItemIsEnabled);

        // Build tree structure first (items must be in the tree before
        // setItemWidget and setExpanded can work)
        QVector<QTreeWidgetItem*> file_items;
        QVector<QTreeWidgetItem*> combo_items;
        for (int i = 0; i < files.size(); i++)
        {
            const BatchFileInfo& info = files[i];

            // File row: filename | status | encoding
            QTreeWidgetItem* file_item = new QTreeWidgetItem;
            file_item->setText(0, info.filename);

            if (info.skip)
            {
                file_item->setText(1, "Skip");
                file_item->setForeground(1, QColor("#DAA520"));
                file_item->setToolTip(1, info.skipReason);
            }
            else if (info.processed && info.processedOk)
            {
                file_item->setText(1, "Done");
                file_item->setForeground(1, QColor("green"));
            }
            else if (info.processed && !info.processedOk)
            {
                file_item->setText(1, "Error");
                file_item->setForeground(1, QColor("red"));
            }
            else if (info.preScanOk)
            {
                file_item->setText(1, "Valid");
                file_item->setForeground(1, QColor("green"));
            }
            else
            {
                file_item->setText(1, "Ready");
                file_item->setForeground(1, QColor("green"));
            }

            if (info.preScanOk)
                file_item->setText(2, info.isRandomized ? "RNRZ-L" : "NRZ-L");
            else
                file_item->setText(2, QString::fromUtf8("\xe2\x80\x94"));

            root->addChild(file_item);

            // Time and PCM combo rows (children of file item) — widgets added after tree insertion
            QTreeWidgetItem* time_item = new QTreeWidgetItem;
            time_item->setFlags(Qt::ItemIsEnabled);
            file_item->addChild(time_item);

            QTreeWidgetItem* pcm_item = new QTreeWidgetItem;
            pcm_item->setFlags(Qt::ItemIsEnabled);
            file_item->addChild(pcm_item);

            file_items.append(file_item);
            combo_items.append(time_item);
            combo_items.append(pcm_item);
        }

        // Insert tree into widget — setItemWidget and setExpanded require this
        m_file_list->addTopLevelItem(root);
        root->setExpanded(true);

        // Now attach combo box widgets and expand file items
        for (int i = 0; i < files.size(); i++)
        {
            const BatchFileInfo& info = files[i];
            QTreeWidgetItem* time_item = combo_items[i * 2];
            QTreeWidgetItem* pcm_item  = combo_items[i * 2 + 1];
            file_items[i]->setExpanded(true);

            // Time channel combo — own row, column 0
            QWidget* time_container = new QWidget;
            time_container->setAttribute(Qt::WA_TranslucentBackground);
            QHBoxLayout* time_layout = new QHBoxLayout(time_container);
            time_layout->setContentsMargins(0, 0, 4, 0);
            time_layout->addWidget(new QLabel("Time:"));
            QComboBox* time_combo = new QComboBox;
            time_combo->addItems(info.timeChannelStrings);
            if (info.resolvedTimeIndex >= 0 && info.resolvedTimeIndex < info.timeChannelStrings.size())
            {
                QSignalBlocker blocker(time_combo);
                time_combo->setCurrentIndex(info.resolvedTimeIndex);
            }
            if (info.timeChannelStrings.isEmpty())
                time_combo->setEnabled(false);
            connect(time_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this, i](int idx) { m_view_model->setBatchFileTimeChannel(i, idx); });
            time_layout->addWidget(time_combo, 1);
            m_file_list->setItemWidget(time_item, 0, time_container);

            // PCM channel combo — own row, column 0
            QWidget* pcm_container = new QWidget;
            pcm_container->setAttribute(Qt::WA_TranslucentBackground);
            QHBoxLayout* pcm_layout = new QHBoxLayout(pcm_container);
            pcm_layout->setContentsMargins(0, 0, 4, 0);
            pcm_layout->addWidget(new QLabel("PCM:"));
            QComboBox* pcm_combo = new QComboBox;
            pcm_combo->addItems(info.pcmChannelStrings);
            if (info.resolvedPcmIndex >= 0 && info.resolvedPcmIndex < info.pcmChannelStrings.size())
            {
                QSignalBlocker blocker(pcm_combo);
                pcm_combo->setCurrentIndex(info.resolvedPcmIndex);
            }
            if (info.pcmChannelStrings.isEmpty())
                pcm_combo->setEnabled(false);
            connect(pcm_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this, i](int idx) { m_view_model->setBatchFilePcmChannel(i, idx); });
            pcm_layout->addWidget(pcm_combo, 1);
            m_file_list->setItemWidget(pcm_item, 0, pcm_container);
        }

        m_file_list->header()->setStretchLastSection(true);
        m_file_list->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_file_list->resizeColumnToContents(0);
        m_file_list->resizeColumnToContents(1);
        m_file_list->resizeColumnToContents(2);
    }
    else
    {
        // Single-file or no-file mode
        m_file_list->setHeaderHidden(true);

        QTreeWidgetItem* file_item = new QTreeWidgetItem;
        if (m_view_model->fileLoaded())
            file_item->setText(0, m_view_model->inputFilename());
        else
            file_item->setText(0, "No file loaded");
        file_item->setFlags(Qt::ItemIsEnabled);

        // Add Time and PCM combo child rows
        QTreeWidgetItem* time_item = new QTreeWidgetItem;
        time_item->setFlags(Qt::ItemIsEnabled);
        file_item->addChild(time_item);

        QTreeWidgetItem* pcm_item = new QTreeWidgetItem;
        pcm_item->setFlags(Qt::ItemIsEnabled);
        file_item->addChild(pcm_item);

        m_file_list->addTopLevelItem(file_item);

        // Attach combo widgets now that items are in the tree
        QStringList time_channels = m_view_model->timeChannelList();
        QStringList pcm_channels = m_view_model->pcmChannelList();
        bool loaded = m_view_model->fileLoaded();

        // Time channel combo
        QWidget* time_container = new QWidget;
        time_container->setAttribute(Qt::WA_TranslucentBackground);
        QHBoxLayout* time_layout = new QHBoxLayout(time_container);
        time_layout->setContentsMargins(0, 0, 4, 0);
        time_layout->addWidget(new QLabel("Time:"));
        QComboBox* time_combo = new QComboBox;
        time_combo->addItems(time_channels);
        time_combo->setEnabled(loaded && !time_channels.isEmpty());
        if (loaded && !time_channels.isEmpty())
        {
            // Auto-select first channel; +1 offset for ViewModel placeholder convention
            int idx = (m_view_model->timeChannelIndex() > 0)
                          ? m_view_model->timeChannelIndex() - 1 : 0;
            QSignalBlocker blocker(time_combo);
            time_combo->setCurrentIndex(idx);
        }
        connect(time_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) { m_view_model->setTimeChannelIndex(idx + 1); });
        time_layout->addWidget(time_combo, 1);
        m_file_list->setItemWidget(time_item, 0, time_container);

        // PCM channel combo
        QWidget* pcm_container = new QWidget;
        pcm_container->setAttribute(Qt::WA_TranslucentBackground);
        QHBoxLayout* pcm_layout = new QHBoxLayout(pcm_container);
        pcm_layout->setContentsMargins(0, 0, 4, 0);
        pcm_layout->addWidget(new QLabel("PCM:"));
        QComboBox* pcm_combo = new QComboBox;
        pcm_combo->addItems(pcm_channels);
        pcm_combo->setEnabled(loaded && !pcm_channels.isEmpty());
        if (loaded && !pcm_channels.isEmpty())
        {
            int idx = (m_view_model->pcmChannelIndex() > 0)
                          ? m_view_model->pcmChannelIndex() - 1 : 0;
            QSignalBlocker blocker(pcm_combo);
            pcm_combo->setCurrentIndex(idx);
        }
        connect(pcm_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) { m_view_model->setPcmChannelIndex(idx + 1); });
        pcm_layout->addWidget(pcm_combo, 1);
        m_file_list->setItemWidget(pcm_item, 0, pcm_container);

        file_item->setExpanded(loaded);

        m_file_list->header()->setStretchLastSection(false);
        m_file_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
}

void MainView::saveLastBatchOutputDir()
{
    QSettings app_settings;
    app_settings.setValue(UIConstants::kSettingsKeyLastBatchDir, m_last_batch_output_dir);
}
