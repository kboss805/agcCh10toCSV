/**
 * @file mainview.h
 * @brief Application main window — thin View layer delegating to MainViewModel.
 */

#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMimeData>
#include <QTextBrowser>
#include <QMenuBar>
#include <QProgressBar>
#include <QScrollBar>
#include <QStringList>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

class MainViewModel;
class PlotViewModel;
class PlotWidget;
class ReceiverGridWidget;
class TimeExtractionWidget;

/**
 * @brief Thin View layer: builds widgets, connects to ViewModel signals,
 *        and forwards user actions to MainViewModel.
 *
 * Delegates receiver grid to ReceiverGridWidget and time controls to
 * TimeExtractionWidget.
 */
class MainView : public QMainWindow
{
    Q_OBJECT

public:
    /// @param[in] parent Optional parent widget.
    MainView(QWidget *parent = nullptr);
    ~MainView();

private slots:
    /// @name User-initiated action slots
    /// @{
    /// Shows a message box with the given error text.
    void displayErrorMessage(const QString& message);
    /// Opens a file dialog and loads the selected .ch10 file.
    void inputFileButtonPressed();
    /// Opens the SettingsDialog for editing settings.
    void onSettings();
    /// Toggles between light and dark themes.
    void onToggleTheme();
    /// Validates inputs and starts background processing.
    void progressProcessButtonPressed();
    /// @}

    /// @name ViewModel-driven update slots
    /// @{
    /// Refreshes the time and PCM channel combo boxes.
    void onChannelListsChanged();
    /// Enables or disables controls based on file-loaded state.
    void onFileLoadedChanged();
    /// Fills the start/stop time fields from the loaded file.
    void onFileTimesChanged();
    /// Updates the progress bar value.
    void onProgressChanged();
    /// Updates UI state when processing starts or stops.
    void onProcessingChanged();
    /// Handles completion of background processing.
    void onProcessingFinished(bool success, const QString& output_file);
    /// Appends a message to the log window.
    void onLogMessage(const QString& message);
    /// @}

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    /// @name Widget setup helpers
    /// @{
    void setUpMenuBar();                     ///< Creates the menu bar.
    void setUpMainLayout();                  ///< Creates the top-level layout and log dialog.
    void setUpFileList();                    ///< Creates the collapsible file list tree widget.
    void setUpConnections();                 ///< Connects all ViewModel signals to View slots.
    /// @}

    /// @name Bulk state helpers
    /// @{
    void setAllControlsEnabled(bool enabled);          ///< Enables or disables all interactive controls.
    void saveLastCh10Dir();                              ///< Persists m_last_ch10_dir to QSettings.
    void saveLastCsvDir();                               ///< Persists m_last_csv_dir to QSettings.
    void logError(const QString& message);               ///< Appends a red error entry to the log window.
    void logWarning(const QString& message);             ///< Appends a dark-yellow warning entry to the log window.
    void logSuccess(const QString& message);             ///< Appends a green success entry to the log window.
    void updateStatusBar();                              ///< Refreshes the status bar from the ViewModel.
    void updateSettingsSummary();                         ///< Refreshes the settings summary label.
    void updateRecentFilesMenu();                        ///< Rebuilds the Recent Files submenu.
    void updateFileList();                                ///< Refreshes the file list tree from ViewModel state.
    void saveLastBatchOutputDir();                        ///< Persists m_last_batch_output_dir to QSettings.
    void onShowPlot(const QString& csv_filepath);         ///< Loads CSV into plot and shows the plot dock.
    /// @}

    MainViewModel* m_view_model;             ///< Owning ViewModel instance.

    QVBoxLayout* m_controls_layout;          ///< Left-side vertical controls layout.
    QDockWidget* m_controls_dock;            ///< Fixed left dock for controls panel.
    QDialog* m_log_dialog;                   ///< Standalone log dialog window.
    QTextBrowser* m_log_window;              ///< Log output pane.
    PlotWidget* m_plot_widget;               ///< Plot view widget (central widget).
    PlotViewModel* m_plot_view_model;        ///< Plot ViewModel owning series data.
    QAction* m_theme_action;                 ///< File > Toggle theme action.
    QAction* m_show_log_action;              ///< View > Show Log action.

    QToolBar* m_toolbar;                     ///< Main toolbar.
    QAction* m_toolbar_open_action;          ///< Toolbar open action.
    QAction* m_process_action;               ///< Toolbar process/play action.
    QAction* m_cancel_action;                ///< Toolbar cancel/stop action (visible during processing).
    QAction* m_toggle_log_action;            ///< Toolbar toggle log dialog visibility.

    QTreeWidget* m_file_list;                 ///< File list tree with per-file channel combo boxes.

    ReceiverGridWidget* m_receiver_grid;     ///< Receiver/channel selection grid.
    TimeExtractionWidget* m_time_widget;     ///< Time extraction and sample rate controls.

    QTextBrowser* m_log_preview;             ///< Compact log preview in the controls panel.
    QProgressBar* m_progress_bar;            ///< Processing progress bar.
    QTreeWidget* m_settings_tree;            ///< Collapsible settings summary tree.
    QMenu* m_recent_menu;                    ///< File > Recent Files submenu.

    QString m_last_ch10_dir;                 ///< Last directory used in Ch10 file dialogs.
    QString m_last_csv_dir;                  ///< Last directory used in CSV file dialogs.
    QString m_last_batch_output_dir;         ///< Last directory used for batch output.
};
#endif // MAINVIEW_H
