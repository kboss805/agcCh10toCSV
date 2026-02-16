/**
 * @file plotviewmodel.cpp
 * @brief Implementation of PlotViewModel — CSV parsing, series data, axis state.
 */

#include "plotviewmodel.h"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QMap>
#include <QTextStream>

#include "constants.h"

PlotViewModel::PlotViewModel(QObject* parent)
    : QObject(parent)
    , m_plot_title(PlotConstants::kDefaultPlotTitle)
{
}

bool PlotViewModel::loadCsvFile(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream stream(&file);

    // Parse header line: "Day,Time,param1,param2,..."
    QString header_line = stream.readLine();
    if (header_line.isEmpty())
        return false;

    QStringList columns = header_line.split(',');
    if (columns.size() < 3)
        return false;

    // Build series list from columns 2..N (skip Day, Time)
    int param_count = columns.size() - 2;
    QVector<PlotSeriesData> series(param_count);

    // Count channels per receiver for shade assignment
    QMap<int, int> receiver_channel_count;

    for (int i = 0; i < param_count; i++)
    {
        PlotSeriesData& s = series[i];
        s.name = columns[i + 2].trimmed();

        // Extract receiver index from "_RCVR<N>" suffix
        int rcvr_pos = s.name.lastIndexOf("_RCVR");
        if (rcvr_pos >= 0)
        {
            bool ok = false;
            int rcvr_num = s.name.mid(rcvr_pos + 5).toInt(&ok);
            s.receiverIndex = ok ? rcvr_num : 0;
        }

        s.channelIndex = receiver_channel_count.value(s.receiverIndex, 0);
        receiver_channel_count[s.receiverIndex]++;
    }

    // Estimate row count from remaining file size for pre-allocation
    qint64 remaining_bytes = file.size() - stream.pos();
    int estimated_rows = static_cast<int>(remaining_bytes / (param_count * 8 + 20));
    if (estimated_rows < 100) estimated_rows = 100;
    for (int i = 0; i < param_count; i++)
    {
        series[i].xValues.reserve(estimated_rows);
        series[i].yValues.reserve(estimated_rows);
    }

    // Parse data rows
    double first_time = -1.0;
    int first_day = 0;

    while (!stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.isEmpty())
            continue;

        QStringList fields = line.split(',');
        if (fields.size() < param_count + 2)
            continue;

        int day = fields[0].toInt();
        double time_seconds = parseTimeToSeconds(fields[1]);

        // Convert DOY + time to elapsed seconds from first sample
        if (first_time < 0.0)
        {
            first_day = day;
            first_time = time_seconds;
            m_base_day = day;
            m_base_time_offset = time_seconds;
        }

        double elapsed = (day - first_day) * UIConstants::kSecondsPerDay
                         + (time_seconds - first_time);

        for (int i = 0; i < param_count; i++)
        {
            bool ok = false;
            double value = fields[i + 2].toDouble(&ok);
            if (ok)
            {
                series[i].xValues.append(elapsed);
                series[i].yValues.append(value);
                if (value < series[i].yMinCached) series[i].yMinCached = value;
                if (value > series[i].yMaxCached) series[i].yMaxCached = value;
            }
        }
    }

    file.close();

    // Verify we got data
    bool has_any_data = false;
    for (const auto& s : series)
    {
        if (!s.xValues.isEmpty())
        {
            has_any_data = true;
            break;
        }
    }

    if (!has_any_data)
        return false;

    m_series = std::move(series);

    // Compute X range from data
    m_x_min = 0.0;
    m_x_max = 0.0;
    for (const auto& s : m_series)
    {
        if (!s.xValues.isEmpty() && s.xValues.last() > m_x_max)
            m_x_max = s.xValues.last();
    }
    m_x_view_min = m_x_min;
    m_x_view_max = m_x_max;

    assignColors();
    computeYRange();

    emit dataChanged();
    return true;
}

void PlotViewModel::clearData()
{
    m_series.clear();
    m_x_min = m_x_max = 0.0;
    m_x_view_min = m_x_view_max = 0.0;
    m_data_y_min = m_data_y_max = 0.0;
    m_y_auto_scale = true;
    m_base_day = 0;
    m_base_time_offset = 0.0;
    m_plot_title = PlotConstants::kDefaultPlotTitle;
    emit dataChanged();
}

bool PlotViewModel::hasData() const
{
    return !m_series.isEmpty();
}

int PlotViewModel::seriesCount() const
{
    return m_series.size();
}

const PlotSeriesData& PlotViewModel::seriesAt(int index) const
{
    return m_series.at(index);
}

const QVector<PlotSeriesData>& PlotViewModel::allSeries() const
{
    return m_series;
}

QString PlotViewModel::plotTitle() const
{
    return m_plot_title;
}

double PlotViewModel::xMin() const
{
    return m_x_min;
}

double PlotViewModel::xMax() const
{
    return m_x_max;
}

double PlotViewModel::yMin() const
{
    return m_y_auto_scale ? m_data_y_min : m_y_manual_min;
}

double PlotViewModel::yMax() const
{
    return m_y_auto_scale ? m_data_y_max : m_y_manual_max;
}

double PlotViewModel::dataYMin() const
{
    return m_data_y_min;
}

double PlotViewModel::dataYMax() const
{
    return m_data_y_max;
}

bool PlotViewModel::yAutoScale() const
{
    return m_y_auto_scale;
}

double PlotViewModel::xViewMin() const
{
    return m_x_view_min;
}

double PlotViewModel::xViewMax() const
{
    return m_x_view_max;
}

void PlotViewModel::setSeriesVisible(int index, bool visible)
{
    if (index < 0 || index >= m_series.size())
        return;
    if (m_series[index].visible == visible)
        return;

    m_series[index].visible = visible;
    emit seriesVisibilityChanged(index);

    if (m_y_auto_scale)
    {
        computeYRange();
        emit axisRangeChanged();
    }
}

void PlotViewModel::setPlotTitle(const QString& title)
{
    if (m_plot_title == title)
        return;
    m_plot_title = title;
    emit plotTitleChanged();
}

void PlotViewModel::setYManualRange(double min, double max)
{
    m_y_manual_min = min;
    m_y_manual_max = max;
    m_y_auto_scale = false;
    emit axisRangeChanged();
}

void PlotViewModel::setYAutoScale(bool enabled)
{
    if (m_y_auto_scale == enabled)
        return;
    m_y_auto_scale = enabled;
    if (enabled)
        computeYRange();
    emit axisRangeChanged();
}

void PlotViewModel::setXViewRange(double min, double max)
{
    m_x_view_min = min;
    m_x_view_max = max;
    emit axisRangeChanged();
}

void PlotViewModel::resetXRange()
{
    m_x_view_min = m_x_min;
    m_x_view_max = m_x_max;
    emit axisRangeChanged();
}

void PlotViewModel::resetYRange()
{
    m_y_auto_scale = true;
    computeYRange();
    emit axisRangeChanged();
}

void PlotViewModel::assignColors()
{
    for (auto& s : m_series)
    {
        int color_idx = (s.receiverIndex - 1) % PlotConstants::kNumReceiverColors;
        if (color_idx < 0) color_idx = 0;

        QColor base = PlotConstants::kReceiverColors[color_idx];

        // Vary saturation for channels within the same receiver
        if (s.channelIndex > 0)
        {
            int h, sat, val;
            base.getHsv(&h, &sat, &val);
            // Reduce saturation by 25% per subsequent channel, minimum 40
            sat = std::max(40, sat - s.channelIndex * 60);
            // Increase value slightly for lighter shade
            val = std::min(255, val + s.channelIndex * 20);
            base.setHsv(h, sat, val);
        }

        s.color = base;
    }
}

void PlotViewModel::computeYRange()
{
    double y_min = std::numeric_limits<double>::max();
    double y_max = std::numeric_limits<double>::lowest();
    bool has_visible = false;

    for (const auto& s : m_series)
    {
        if (!s.visible || s.yValues.isEmpty())
            continue;

        has_visible = true;
        if (s.yMinCached < y_min) y_min = s.yMinCached;
        if (s.yMaxCached > y_max) y_max = s.yMaxCached;
    }

    if (!has_visible)
    {
        m_data_y_min = 0.0;
        m_data_y_max = 1.0;
        return;
    }

    // Round to nearest 5 dB, clip min at 0
    m_data_y_min = std::max(0.0, std::floor(y_min / 5.0) * 5.0);
    m_data_y_max = std::ceil(y_max / 5.0) * 5.0;
    if (m_data_y_max <= m_data_y_min)
        m_data_y_max = m_data_y_min + 5.0;
}

double PlotViewModel::parseTimeToSeconds(const QString& time_str)
{
    // Format: "HH:MM:SS.mmm"
    QStringList parts = time_str.split(':');
    if (parts.size() != 3)
        return 0.0;

    int hours = parts[0].toInt();
    int minutes = parts[1].toInt();

    // Seconds may include milliseconds: "SS.mmm"
    double seconds = parts[2].toDouble();

    return hours * UIConstants::kSecondsPerHour
           + minutes * UIConstants::kSecondsPerMinute
           + seconds;
}
