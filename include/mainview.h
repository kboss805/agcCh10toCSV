/**
 * @file mainview.h
 * @brief Application main window — thin View layer delegating to MainViewModel.
 */

#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVector>

class MainViewModel;

/**
 * @brief Thin View layer: builds widgets, connects to ViewModel signals,
 *        and forwards user actions to MainViewModel.
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
    /// Opens the ConfigDialog for editing settings.
    void onSettings();
    /// Toggles extraction of the full time range or a user-defined window.
    void timeAllCheckBoxToggled(bool checked);
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
    /// Rebuilds the receiver checkbox grid after layout changes.
    void onReceiverLayoutChanged();
    /// Syncs a single receiver checkbox from the ViewModel.
    void onReceiverCheckedChanged(int receiver_index, int channel_index, bool checked);
    /// Forwards tree item check-state changes to the ViewModel.
    void onTreeItemChanged(QTreeWidgetItem* item, int column);
    /// @}

private:
    /// @name Widget setup helpers
    /// @{
    void setUpMenuBar();                     ///< Creates the menu bar with File menu actions.
    void setUpMainLayout();                  ///< Creates the top-level horizontal layout.
    void setUpTimeChannelRow();              ///< Creates the time channel combo box row.
    void setUpPCMChannelRow();               ///< Creates the PCM channel combo box row.

    void setUpReceiversSection();            ///< Creates the receivers group box and "All" checkbox.
    void rebuildReceiversGrid();             ///< Rebuilds the receiver checkbox grid from ViewModel state.

    void setUpTimeSection();                 ///< Creates the time extraction group box.
    void setUpTimeSectionRow1();             ///< Creates the "All time" checkbox row.
    void setUpTimeSectionRow2();             ///< Creates the start/stop time input rows.
    void setUpTimeSectionRow2StartTime();    ///< Creates the start time input fields.
    void setUpTimeSectionRow2StopTime();     ///< Creates the stop time input fields.
    /// Creates a labeled group of DDD:HH:MM:SS input fields.
    void setUpTimeInputGroup(const QString& label, QVBoxLayout*& section,
                             QLineEdit*& ddd, QLineEdit*& hh, QLineEdit*& mm, QLineEdit*& ss);
    void setUpTimeSectionRow3();             ///< Creates the sample rate combo box row.

    void setUpConnections();                 ///< Connects all ViewModel signals to View slots.
    /// @}

    /// @name Bulk state helpers
    /// @{
    void setAllControlsEnabled(bool enabled);          ///< Enables or disables all interactive controls.
    void setAllNumberedReceiversEnabled(bool enabled);  ///< Enables or disables individual receiver checkboxes.
    void setAllNumberedReceiversChecked(bool checked);  ///< Checks or unchecks all individual receiver checkboxes.

    void setAllStartStopTimesEnabled(bool enabled);     ///< Enables or disables start/stop time fields.
    void fillAllStartStopTimes();                       ///< Populates time fields from ViewModel file times.
    void clearAllStartStopTimes();                      ///< Clears all start/stop time fields.
    /// @}

    MainViewModel* m_view_model;             ///< Owning ViewModel instance.

    QWidget* m_central_widget;               ///< Central widget container.
    QHBoxLayout* m_central_layout;           ///< Top-level horizontal layout.
    QVBoxLayout* m_controls_layout;          ///< Left-side vertical controls layout.
    QPlainTextEdit* m_log_window;            ///< Right-side log output pane.

    QAction* m_open_file_action;             ///< File > Open action.

    QHBoxLayout* m_time_channel_layout;      ///< Time channel row layout.
    QComboBox* m_time_channel;               ///< Time channel selector.

    QHBoxLayout* m_pcm_channel_layout;       ///< PCM channel row layout.
    QComboBox* m_pcm_channel;                ///< PCM channel selector.

    QGroupBox* m_receivers_section;          ///< Receivers group box.
    QVBoxLayout* m_receivers_section_layout; ///< Receivers section vertical layout.
    QVector<QTreeWidget*> m_receiver_trees;  ///< Column trees for receiver display.

    QGroupBox* m_time_section;               ///< Time extraction group box.
    QHBoxLayout* m_time_all_layout;          ///< "All time" checkbox row layout.
    QCheckBox* m_time_all;                   ///< "Extract all time" toggle checkbox.

    QVBoxLayout* m_time_start_stop_layout;   ///< Container for start/stop time groups.
    QVBoxLayout* m_time_start_section;       ///< Start time input group layout.
    QLineEdit* m_start_ddd;                  ///< Start day-of-year input.
    QLineEdit* m_start_hh;                   ///< Start hour input.
    QLineEdit* m_start_mm;                   ///< Start minute input.
    QLineEdit* m_start_ss;                   ///< Start second input.
    QVBoxLayout* m_time_stop_section;        ///< Stop time input group layout.
    QLineEdit* m_stop_ddd;                   ///< Stop day-of-year input.
    QLineEdit* m_stop_hh;                    ///< Stop hour input.
    QLineEdit* m_stop_mm;                    ///< Stop minute input.
    QLineEdit* m_stop_ss;                    ///< Stop second input.

    QHBoxLayout* m_sample_rate_layout;       ///< Sample rate row layout.
    QComboBox* m_sample_rate;                ///< Sample rate selector.

    QHBoxLayout* m_progress_bar_layout;      ///< Progress/process button row layout.
    QProgressBar* m_progress_bar;            ///< Processing progress bar.
    QPushButton* m_process_btn;              ///< Start/stop processing button.

    QString m_last_dir;                      ///< Last directory used in file dialogs.

    bool m_updating_from_viewmodel;          ///< Guard flag to prevent signal loops during ViewModel sync.
};
#endif // MAINVIEW_H
