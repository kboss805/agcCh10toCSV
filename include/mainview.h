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
    MainView(QWidget *parent = nullptr);
    ~MainView();

private slots:
    void displayErrorMessage(const QString& message);

    // User-initiated actions
    void inputFileButtonPressed();
    void onSettings();
    void receiversAllCheckBoxToggled(bool checked);
    void timeAllCheckBoxToggled(bool checked);
    void progressProcessButtonPressed();

    // ViewModel-driven updates
    void onChannelListsChanged();
    void onFileLoadedChanged();
    void onFileTimesChanged();
    void onProgressChanged();
    void onProcessingChanged();
    void onProcessingFinished(bool success, const QString& output_file);
    void onLogMessage(const QString& message);
    void onReceiverLayoutChanged();
    void onReceiverCheckedChanged(int receiver_index, int channel_index, bool checked);

private:
    void setUpMenuBar();
    void setUpMainLayout();
    void setUpTimeChannelRow();
    void setUpPCMChannelRow();

    void setUpReceiversSection();
    void rebuildReceiversGrid();

    void setUpTimeSection();
    void setUpTimeSectionRow1();
    void setUpTimeSectionRow2();
    void setUpTimeSectionRow2StartTime();
    void setUpTimeSectionRow2StopTime();
    void setUpTimeInputGroup(const QString& label, QVBoxLayout*& section,
                             QLineEdit*& ddd, QLineEdit*& hh, QLineEdit*& mm, QLineEdit*& ss);
    void setUpTimeSectionRow3();

    void setUpConnections();

    void setAllControlsEnabled(bool enabled);
    void setAllNumberedReceiversEnabled(bool enabled);
    void setAllNumberedReceiversChecked(bool checked);

    void setAllStartStopTimesEnabled(bool enabled);
    void fillAllStartStopTimes();
    void clearAllStartStopTimes();

    MainViewModel* m_view_model;

    QWidget* m_central_widget;
    QHBoxLayout* m_central_layout;
    QVBoxLayout* m_controls_layout;
    QPlainTextEdit* m_log_window;

    QAction* m_open_file_action;

    QHBoxLayout* m_time_channel_layout;
    QComboBox* m_time_channel;

    QHBoxLayout* m_pcm_channel_layout;
    QComboBox* m_pcm_channel;

    QGroupBox* m_receivers_section;
    QVBoxLayout* m_receivers_section_layout;
    QCheckBox* m_receivers_all;
    QVector<QVector<QCheckBox*>> m_receiver_checks;

    QGroupBox* m_time_section;
    QHBoxLayout* m_time_all_layout;
    QCheckBox* m_time_all;

    QVBoxLayout* m_time_start_stop_layout;
    QVBoxLayout* m_time_start_section;
    QLineEdit* m_start_ddd;
    QLineEdit* m_start_hh;
    QLineEdit* m_start_mm;
    QLineEdit* m_start_ss;
    QVBoxLayout* m_time_stop_section;
    QLineEdit* m_stop_ddd;
    QLineEdit* m_stop_hh;
    QLineEdit* m_stop_mm;
    QLineEdit* m_stop_ss;

    QHBoxLayout* m_sample_rate_layout;
    QComboBox* m_sample_rate;

    QHBoxLayout* m_progress_bar_layout;
    QProgressBar* m_progress_bar;
    QPushButton* m_process_btn;

    QString m_last_dir;

    bool m_updating_from_viewmodel;
};
#endif // MAINVIEW_H
