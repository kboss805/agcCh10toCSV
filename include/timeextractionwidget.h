/**
 * @file timeextractionwidget.h
 * @brief Widget with time range selection and sample rate controls.
 */

#ifndef TIMEEXTRACTIONWIDGET_H
#define TIMEEXTRACTIONWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>

#include "constants.h"

/**
 * @brief Widget containing the extract-all toggle, start/stop time inputs,
 *        and sample rate selector.
 */
class TimeExtractionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TimeExtractionWidget(QWidget* parent = nullptr);

    bool extractAllTime() const;                 ///< @return True if "Extract All Time" is checked.
    void setExtractAllTime(bool value);           ///< Sets the "Extract All Time" checkbox.

    int sampleRateIndex() const;                 ///< @return Sample rate combo box index.
    void setSampleRateIndex(int index);           ///< Sets the sample rate combo box index.

    /// Enables or disables all controls in the widget.
    void setAllEnabled(bool enabled);

    /// Enables or disables just the sample rate selector.
    void setSampleRateEnabled(bool enabled);

    /**
     * @brief Populates start/stop time fields from file times.
     * @param[in] start_ddd Start day-of-year.
     * @param[in] start_hh  Start hour.
     * @param[in] start_mm  Start minute.
     * @param[in] start_ss  Start second.
     * @param[in] stop_ddd  Stop day-of-year.
     * @param[in] stop_hh   Stop hour.
     * @param[in] stop_mm   Stop minute.
     * @param[in] stop_ss   Stop second.
     */
    void fillTimes(int start_ddd, int start_hh, int start_mm, int start_ss,
                   int stop_ddd, int stop_hh, int stop_mm, int stop_ss);

    /// Clears all start/stop time fields.
    void clearTimes();

    QString startTimeText() const;               ///< @return Start time text in "DDD:HH:MM:SS" format.
    QString stopTimeText() const;                ///< @return Stop time text in "DDD:HH:MM:SS" format.

signals:
    void extractAllTimeChanged(bool checked);    ///< Emitted when the "Extract All Time" checkbox is toggled.
    void sampleRateIndexChanged(int index);      ///< Emitted when the sample rate selection changes.

private:
    QCheckBox* m_time_all;                       ///< "Extract all time" toggle checkbox.
    QLineEdit* m_start_time;                     ///< Start time input (DDD:HH:MM:SS).
    QLineEdit* m_stop_time;                      ///< Stop time input (DDD:HH:MM:SS).
    QComboBox* m_sample_rate;                    ///< Sample rate selector.
};

#endif // TIMEEXTRACTIONWIDGET_H
