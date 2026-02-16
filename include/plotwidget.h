/**
 * @file plotwidget.h
 * @brief View widget for AGC signal plot — QCustomPlot chart with controls.
 */

#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <functional>

#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

class QCustomPlot;
class QCPGraph;
class PlotViewModel;

/**
 * @brief Self-contained plot widget: QCustomPlot chart + toolbar controls + legend.
 *
 * Owns its own QCustomPlot instance. Connects to a PlotViewModel for data.
 * Placed inside a QDockWidget by MainView.
 */
class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget* parent = nullptr);

    /// Connects this widget to a PlotViewModel instance.
    void setViewModel(PlotViewModel* vm);

    /// Applies theme colors (dark/light) to the chart.
    void applyTheme(bool dark);

    /// Builds the receiver legend tree from settings (disabled until data loads).
    void initReceiverLegend(int receiver_count, int channels_per_receiver,
                            const std::function<QString(int)>& channel_prefix_fn);

public slots:
    /// Rebuilds all chart series from the ViewModel data (no legend rebuild).
    void rebuildChart();

private slots:
    /// Called when new CSV data is loaded — rebuilds both chart and legend.
    void onDataChanged();
    /// Toggles a single graph's visibility without full rebuild.
    void onSeriesVisibilityToggled(int index);
    /// Syncs axis ranges from ViewModel to the QCustomPlot axes.
    void updateAxes();
    /// Updates chart title from ViewModel.
    void updateTitle();
    /// Handles a series visibility checkbox toggle.
    void onLegendCheckboxToggled(int series_index, bool checked);
    /// Handles user editing manual Y range spinboxes.
    void onManualYChanged();
    /// Handles user editing X range spinboxes.
    void onXRangeChanged();
    /// Resets all axes to auto/full range.
    void onResetAxes();

private:
    /// Handles QCustomPlot axis range change from mouse interaction.
    void handlePlotXRangeChanged(double lower, double upper);
    /// Handles QCustomPlot Y axis range change from mouse interaction.
    void handlePlotYRangeChanged(double lower, double upper);
    void setUpLayout();
    void setUpConnections();
    void rebuildLegend();

    PlotViewModel* m_view_model = nullptr;

    /// @name Chart
    /// @{
    QCustomPlot* m_plot;
    /// @}

    /// @name Top toolbar controls
    /// @{
    QLineEdit* m_title_edit;
    QDoubleSpinBox* m_y_min_spin;
    QDoubleSpinBox* m_y_max_spin;
    /// @}

    /// @name Bottom controls
    /// @{
    QDoubleSpinBox* m_x_start_spin;
    QDoubleSpinBox* m_x_stop_spin;
    QPushButton* m_reset_btn;
    /// @}

    /// @name Graph tracking
    /// @{
    QVector<QCPGraph*> m_graphs;          ///< Maps series index → QCPGraph pointer.
    /// @}

    /// @name Legend tree panel
    /// @{
    QWidget* m_legend_panel;
    QVBoxLayout* m_legend_layout;
    QVector<QTreeWidget*> m_legend_trees;
    /// @}

    bool m_updating_from_vm = false;  ///< Guard against signal loops.
    QColor m_title_color;              ///< Chart title text color (accent blue).
};

#endif // PLOTWIDGET_H
