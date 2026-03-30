/**
 * @file plotviewmodel.h
 * @brief ViewModel for the AGC signal plot — CSV parsing, series data, axis state.
 */

#ifndef PLOTVIEWMODEL_H
#define PLOTVIEWMODEL_H

#include <limits>

#include <QColor>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QVector>

/**
 * @brief Data for a single plot series (one receiver channel).
 *
 * Built by PlotViewModel::loadCsvFile() from the CSV output.
 */
struct PlotSeriesData
{
    QString name;             ///< Column header, e.g., "L_RCVR1".
    int receiverIndex = 0;    ///< 1-based receiver number from "_RCVR<N>" suffix.
    int channelIndex  = 0;    ///< 0-based within receiver, for color shade.
    QVector<double> xValues;  ///< Elapsed seconds from first sample.
    QVector<double> yValues;  ///< Calibrated dB values.
    bool visible = true;      ///< Whether this series is currently shown.
    QColor color;             ///< Assigned display color.
    double yMinCached = std::numeric_limits<double>::max();    ///< Cached min Y value.
    double yMaxCached = std::numeric_limits<double>::lowest(); ///< Cached max Y value.
};

/**
 * @brief Result of a CSV parse operation.
 *
 * Returned by PlotViewModel::parseCsvData() and carried across the thread
 * boundary by QFutureWatcher.
 */
struct CsvParseResult
{
    bool success = false;
    QVector<PlotSeriesData> series;
    int baseDay = 0;
    double baseTimeOffset = 0.0;
    double xMax = 0.0;
};

/**
 * @brief ViewModel for the AGC signal plot window.
 *
 * Parses a CSV file produced by FrameProcessor, stores all series data
 * in memory, and exposes axis ranges and series visibility for the View.
 */
class PlotViewModel : public QObject
{
    Q_OBJECT

public:
    explicit PlotViewModel(QObject* parent = nullptr);

    /// @name Data loading
    /// @{
    /// Parses the CSV file synchronously and populates series data. Returns true on success.
    bool loadCsvFile(const QString& filepath);
    /// Parses the CSV file on a background thread. Emits loadStarted(), then dataChanged() or loadFailed().
    void loadCsvFileAsync(const QString& filepath);
    /// Resets all data to empty state.
    void clearData();
    /// @}

    /// @name Accessors
    /// @{
    bool hasData() const;                          ///< @return True if series data is loaded.
    bool isLoading() const;                        ///< @return True if an async parse is in progress.
    int seriesCount() const;                       ///< @return Number of loaded series.
    const PlotSeriesData& seriesAt(int index) const; ///< @return Series at the given index.
    const QVector<PlotSeriesData>& allSeries() const; ///< @return All series data.

    QString plotTitle() const;                     ///< @return Current plot title.
    double xMin() const;                           ///< @return Data X minimum (elapsed seconds).
    double xMax() const;                           ///< @return Data X maximum (elapsed seconds).
    double yMin() const;                           ///< @return Current Y minimum (auto or manual).
    double yMax() const;                           ///< @return Current Y maximum (auto or manual).
    double dataYMin() const;                       ///< @return Computed Y minimum from data.
    double dataYMax() const;                       ///< @return Computed Y maximum from data.
    bool yAutoScale() const;                       ///< @return True if Y axis is auto-scaled.

    double xViewMin() const;                       ///< @return Current X viewport minimum.
    double xViewMax() const;                       ///< @return Current X viewport maximum.

    int baseDay() const;                           ///< @return DOY of the first sample.
    double baseTimeOffset() const;                 ///< @return Seconds-since-midnight of first sample.
    /// Converts elapsed seconds to "DDD:HH:MM:SS" using the file's base time.
    QString formatTime(double elapsed) const;
    /// @}

    /// @name Mutators
    /// @{
    void setSeriesVisible(int index, bool visible);
    void setPlotTitle(const QString& title);
    void setYManualRange(double min, double max);
    void setYAutoScale(bool enabled);
    void setXViewRange(double min, double max);
    void resetXRange();
    void resetYRange();
    /// @}

signals:
    void dataChanged();                            ///< Emitted when CSV data is loaded or cleared.
    void loadStarted();                            ///< Emitted when an async load begins.
    void loadFailed();                             ///< Emitted when an async load fails.
    void seriesVisibilityChanged(int index);        ///< Emitted when a series visibility toggles.
    void plotTitleChanged();                        ///< Emitted when the plot title changes.
    void axisRangeChanged();                        ///< Emitted when X or Y axis ranges change.

private slots:
    void onParseFinished();                         ///< Receives result from background parse thread.

private:
    /// Assigns colors to all series based on receiver grouping.
    void assignColors();
    /// Computes Y axis range from visible series data with margin.
    void computeYRange();
    /// Commits a CsvParseResult into member state and emits dataChanged().
    void commitParseResult(CsvParseResult&& result);
    /// Parses a "HH:MM:SS.mmm" time string to seconds since midnight.
    static double parseTimeToSeconds(const QString& time_str);
    /// Parses the CSV data rows into the series structure.
    static void parseCsvDataRows(QTextStream& stream, int param_count,
                                 QVector<PlotSeriesData>& series,
                                 int& out_base_day, double& out_base_time_offset);
    /// Pure parse function — safe to run on any thread.
    static CsvParseResult parseCsvData(const QString& filepath);

    QVector<PlotSeriesData> m_series;              ///< All loaded series data.
    QString m_plot_title;                          ///< User-defined plot title.

    double m_x_min = 0.0;                          ///< Data X range minimum.
    double m_x_max = 0.0;                          ///< Data X range maximum.
    double m_x_view_min = 0.0;                     ///< Current viewport X minimum.
    double m_x_view_max = 0.0;                     ///< Current viewport X maximum.

    double m_data_y_min = 0.0;                     ///< Computed Y minimum from visible data.
    double m_data_y_max = 0.0;                     ///< Computed Y maximum from visible data.
    double m_y_manual_min = 0.0;                   ///< Manual Y minimum override.
    double m_y_manual_max = 0.0;                   ///< Manual Y maximum override.
    bool m_y_auto_scale = true;                    ///< True to auto-scale Y axis.

    int m_base_day = 0;                            ///< DOY of the first sample (for display).
    double m_base_time_offset = 0.0;               ///< Seconds-since-midnight of first sample.

    QFutureWatcher<CsvParseResult>* m_parse_watcher = nullptr; ///< Watcher for async parse future.
    bool m_loading = false;                        ///< True while an async parse is in flight.
};

#endif // PLOTVIEWMODEL_H
