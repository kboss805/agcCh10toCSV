/**
 * @file plotwidget.cpp
 * @brief Implementation of PlotWidget — QCustomPlot chart with controls.
 */

#include "plotwidget.h"

#include <algorithm>

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QVBoxLayout>

#include "qcustomplot.h"

#include "constants.h"
#include "plotviewmodel.h"

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setUpLayout();
    setUpConnections();
}

void PlotWidget::setViewModel(PlotViewModel* vm)
{
    if (m_view_model)
    {
        disconnect(m_view_model, nullptr, this, nullptr);
    }

    m_view_model = vm;
    if (!vm) return;

    connect(vm, &PlotViewModel::dataChanged, this, &PlotWidget::onDataChanged);
    connect(vm, &PlotViewModel::seriesVisibilityChanged, this, &PlotWidget::onSeriesVisibilityToggled);
    connect(vm, &PlotViewModel::axisRangeChanged, this, &PlotWidget::updateAxes);
    connect(vm, &PlotViewModel::plotTitleChanged, this, &PlotWidget::updateTitle);
}

void PlotWidget::applyTheme(bool dark)
{
    QColor bg = dark ? QColor(32, 32, 32) : QColor(255, 255, 255);
    QColor fg = dark ? QColor(220, 220, 220) : QColor(30, 30, 30);
    QColor grid = dark ? QColor(60, 60, 60) : QColor(200, 200, 200);
    m_title_color = dark ? QColor("#60CDFF") : QColor("#005FB8");

    m_plot->setBackground(QBrush(bg));
    m_plot->xAxis->setBasePen(QPen(fg));
    m_plot->yAxis->setBasePen(QPen(fg));
    m_plot->xAxis->setTickPen(QPen(fg));
    m_plot->yAxis->setTickPen(QPen(fg));
    m_plot->xAxis->setSubTickPen(QPen(fg));
    m_plot->yAxis->setSubTickPen(QPen(fg));
    m_plot->xAxis->setTickLabelColor(fg);
    m_plot->yAxis->setTickLabelColor(fg);
    m_plot->xAxis->setLabelColor(fg);
    m_plot->yAxis->setLabelColor(fg);
    m_plot->xAxis->grid()->setPen(QPen(grid, 0, Qt::DotLine));
    m_plot->yAxis->grid()->setPen(QPen(grid, 0, Qt::DotLine));

    // Update existing title element color
    if (m_plot->plotLayout()->elementCount() > 1)
    {
        QCPTextElement* title = qobject_cast<QCPTextElement*>(m_plot->plotLayout()->element(0, 0));
        if (title)
            title->setTextColor(m_title_color);
    }

    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void PlotWidget::initReceiverLegend(int receiver_count, int channels_per_receiver,
                                     const std::function<QString(int)>& channel_prefix_fn)
{
    // Clear existing legend contents
    m_legend_trees.clear();
    QLayoutItem* item;
    while ((item = m_legend_layout->takeAt(0)) != nullptr)
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

    if (receiver_count <= 0)
        return;

    // Button row: Expand All, Select All, Select None (all disabled)
    QHBoxLayout* btn_row = new QHBoxLayout;
    btn_row->setContentsMargins(0, 0, 0, 0);

    QPushButton* toggle_btn = new QPushButton("Expand All");
    toggle_btn->setFlat(true);
    toggle_btn->setEnabled(false);
    QPushButton* select_all_btn = new QPushButton("Select All");
    select_all_btn->setFlat(true);
    select_all_btn->setEnabled(false);
    QPushButton* select_none_btn = new QPushButton("Select None");
    select_none_btn->setFlat(true);
    select_none_btn->setEnabled(false);

    btn_row->addWidget(toggle_btn);
    btn_row->addWidget(select_all_btn);
    btn_row->addWidget(select_none_btn);
    btn_row->addStretch(1);
    m_legend_layout->addLayout(btn_row);

    // Distribute receivers across up to 4 columns
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
        tree->setFixedHeight(per_column * UIConstants::kTreeItemHeightFactor +
                             UIConstants::kTreeHeightBuffer);
        tree->setStyleSheet("QTreeWidget { background: transparent; }");
        tree->setAttribute(Qt::WA_TranslucentBackground);
        tree->setEnabled(false);

        for (int r = start; r < end; r++)
        {
            QTreeWidgetItem* receiver_item = new QTreeWidgetItem;
            receiver_item->setText(0, "RCVR " + QString::number(r + 1));
            receiver_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
            receiver_item->setData(0, Qt::UserRole, -1);

            for (int c = 0; c < channels_per_receiver; c++)
            {
                QTreeWidgetItem* channel_item = new QTreeWidgetItem;
                channel_item->setText(0, channel_prefix_fn(c) + "_RCVR" + QString::number(r + 1));
                channel_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                channel_item->setCheckState(0, Qt::Checked);
                channel_item->setData(0, Qt::UserRole, -1);
                receiver_item->addChild(channel_item);
            }

            tree->addTopLevelItem(receiver_item);
        }

        tree->collapseAll();
        columns_layout->addWidget(tree);
        m_legend_trees.append(tree);
    }

    columns_layout->addStretch(1);
    m_legend_layout->addLayout(columns_layout);

    // Sync scrollbars across columns
    if (m_legend_trees.size() > 1)
    {
        QScrollBar* visible_bar = m_legend_trees.last()->verticalScrollBar();
        for (int i = 0; i < m_legend_trees.size() - 1; i++)
        {
            m_legend_trees[i]->verticalScrollBar()->setFixedWidth(0);
            connect(visible_bar, &QScrollBar::valueChanged,
                    m_legend_trees[i]->verticalScrollBar(), &QScrollBar::setValue);
        }
    }

    // Expand All / Collapse All toggle
    connect(toggle_btn, &QPushButton::clicked, this, [this, toggle_btn]() {
        bool any_collapsed = false;
        for (QTreeWidget* t : m_legend_trees)
            for (int i = 0; i < t->topLevelItemCount(); i++)
                if (!t->topLevelItem(i)->isExpanded())
                    any_collapsed = true;

        for (QTreeWidget* t : m_legend_trees)
        {
            if (any_collapsed)
                t->expandAll();
            else
                t->collapseAll();
        }
        toggle_btn->setText(any_collapsed ? "Collapse All" : "Expand All");
    });
}

void PlotWidget::rebuildChart()
{
    if (!m_view_model) return;

    m_updating_from_vm = true;

    m_plot->clearGraphs();
    m_graphs.clear();

    const auto& all_series = m_view_model->allSeries();
    for (int i = 0; i < all_series.size(); i++)
    {
        const PlotSeriesData& s = all_series[i];
        QCPGraph* graph = m_plot->addGraph();
        graph->setName(s.name);
        graph->setPen(QPen(s.color, 1.5));
        graph->setData(s.xValues, s.yValues, true);
        graph->setVisible(s.visible);
        m_graphs.append(graph);
    }

    // Set axis labels
    m_plot->xAxis->setLabel(PlotConstants::kXAxisLabel);
    m_plot->yAxis->setLabel(PlotConstants::kYAxisLabel);

    // Update title and axes without triggering extra replots
    updateTitle();
    updateAxes();

    // Enable all controls when data is loaded
    bool has_data = m_view_model->hasData();
    m_title_edit->setEnabled(has_data);
    m_x_start_spin->setEnabled(has_data);
    m_x_stop_spin->setEnabled(has_data);
    m_y_min_spin->setEnabled(has_data);
    m_y_max_spin->setEnabled(has_data);
    m_reset_btn->setEnabled(has_data);
    m_plot->setInteractions(has_data
        ? QCP::iRangeDrag | QCP::iRangeZoom
        : QCP::Interactions());

    if (has_data)
    {
        m_x_start_spin->setRange(m_view_model->xMin(), m_view_model->xMax());
        m_x_stop_spin->setRange(m_view_model->xMin(), m_view_model->xMax());
        m_x_start_spin->setValue(m_view_model->xViewMin());
        m_x_stop_spin->setValue(m_view_model->xViewMax());

        m_y_min_spin->setValue(m_view_model->yMin());
        m_y_max_spin->setValue(m_view_model->yMax());
    }

    m_plot->replot(QCustomPlot::rpQueuedReplot);
    m_updating_from_vm = false;
}

void PlotWidget::onDataChanged()
{
    rebuildChart();
    rebuildLegend();
}

void PlotWidget::onSeriesVisibilityToggled(int index)
{
    if (!m_view_model) return;
    if (index < 0 || index >= m_graphs.size()) return;

    m_graphs[index]->setVisible(m_view_model->seriesAt(index).visible);
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void PlotWidget::updateAxes()
{
    if (!m_view_model) return;

    m_updating_from_vm = true;

    m_plot->xAxis->setRange(m_view_model->xViewMin(), m_view_model->xViewMax());
    m_plot->yAxis->setRange(m_view_model->yMin(), m_view_model->yMax());

    m_x_start_spin->setValue(m_view_model->xViewMin());
    m_x_stop_spin->setValue(m_view_model->xViewMax());
    m_y_min_spin->setValue(m_view_model->yMin());
    m_y_max_spin->setValue(m_view_model->yMax());

    m_plot->replot(QCustomPlot::rpQueuedReplot);
    m_updating_from_vm = false;
}

void PlotWidget::updateTitle()
{
    if (!m_view_model) return;

    m_updating_from_vm = true;
    m_title_edit->setText(m_view_model->plotTitle());

    // Show title on chart using a QCPTextElement if one exists, else create one
    if (m_plot->plotLayout()->elementCount() > 1)
    {
        QCPTextElement* title = qobject_cast<QCPTextElement*>(m_plot->plotLayout()->element(0, 0));
        if (title)
        {
            title->setText(m_view_model->plotTitle());
            title->setTextColor(m_title_color);
        }
    }
    else
    {
        QCPTextElement* title = new QCPTextElement(m_plot, m_view_model->plotTitle());
        title->setFont(QFont("sans", 10, QFont::Bold));
        title->setTextColor(m_title_color);
        m_plot->plotLayout()->insertRow(0);
        m_plot->plotLayout()->addElement(0, 0, title);
    }
    m_plot->replot(QCustomPlot::rpQueuedReplot);
    m_updating_from_vm = false;
}

void PlotWidget::onLegendCheckboxToggled(int series_index, bool checked)
{
    if (m_updating_from_vm || !m_view_model) return;
    m_view_model->setSeriesVisible(series_index, checked);
}

void PlotWidget::onManualYChanged()
{
    if (m_updating_from_vm || !m_view_model) return;
    m_view_model->setYManualRange(m_y_min_spin->value(), m_y_max_spin->value());
}

void PlotWidget::onXRangeChanged()
{
    if (m_updating_from_vm || !m_view_model) return;
    double start = qMax(0.0, m_x_start_spin->value());
    m_view_model->setXViewRange(start, m_x_stop_spin->value());
}

void PlotWidget::onResetAxes()
{
    if (!m_view_model) return;
    m_view_model->resetXRange();
    m_view_model->resetYRange();
}

void PlotWidget::handlePlotXRangeChanged(double lower, double upper)
{
    if (m_updating_from_vm || !m_view_model) return;

    double x_max = m_view_model->xMax();
    double width = upper - lower;

    if (lower < 0.0)
    {
        lower = 0.0;
        upper = width;
    }
    if (upper > x_max)
    {
        upper = x_max;
        lower = std::max(0.0, x_max - width);
    }

    m_view_model->setXViewRange(lower, upper);
}

void PlotWidget::handlePlotYRangeChanged(double lower, double upper)
{
    if (m_updating_from_vm || !m_view_model) return;
    m_view_model->setYManualRange(lower, upper);
}

void PlotWidget::setUpLayout()
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 8, 4, 4);
    main_layout->setSpacing(0);

    // --- Title row ---
    auto* title_bar = new QHBoxLayout;
    title_bar->addWidget(new QLabel("Title:"));
    m_title_edit = new QLineEdit;
    m_title_edit->setPlaceholderText(PlotConstants::kDefaultPlotTitle);
    m_title_edit->setEnabled(false);
    title_bar->addWidget(m_title_edit, 1);
    main_layout->addLayout(title_bar);
    main_layout->addSpacing(8);

    // --- QCustomPlot chart ---
    m_plot = new QCustomPlot(this);
    m_plot->setInteractions(QCP::Interactions());
    m_plot->axisRect()->setRangeDrag(Qt::Horizontal);
    m_plot->axisRect()->setRangeZoom(Qt::Horizontal);
    m_plot->xAxis->setLabel(PlotConstants::kXAxisLabel);
    m_plot->yAxis->setLabel(PlotConstants::kYAxisLabel);
    main_layout->addWidget(m_plot, 1);
    main_layout->addSpacing(8);

    // --- Axis controls grid (X row then Y row, columns aligned) ---
    auto* axis_grid = new QGridLayout;
    axis_grid->setContentsMargins(0, 0, 0, 0);

    axis_grid->addWidget(new QLabel("X Start:"), 0, 0);
    m_x_start_spin = new QDoubleSpinBox;
    m_x_start_spin->setRange(0.0, 1e9);
    m_x_start_spin->setDecimals(1);
    m_x_start_spin->setSuffix(" s");
    m_x_start_spin->setEnabled(false);
    axis_grid->addWidget(m_x_start_spin, 0, 1);

    axis_grid->addWidget(new QLabel("X Stop:"), 0, 2);
    m_x_stop_spin = new QDoubleSpinBox;
    m_x_stop_spin->setRange(0.0, 1e9);
    m_x_stop_spin->setDecimals(1);
    m_x_stop_spin->setSuffix(" s");
    m_x_stop_spin->setEnabled(false);
    axis_grid->addWidget(m_x_stop_spin, 0, 3);

    axis_grid->addWidget(new QLabel("Y Min:"), 1, 0);
    m_y_min_spin = new QDoubleSpinBox;
    m_y_min_spin->setRange(0.0, 999.0);
    m_y_min_spin->setDecimals(1);
    m_y_min_spin->setEnabled(false);
    axis_grid->addWidget(m_y_min_spin, 1, 1);

    axis_grid->addWidget(new QLabel("Y Max:"), 1, 2);
    m_y_max_spin = new QDoubleSpinBox;
    m_y_max_spin->setRange(0.0, 999.0);
    m_y_max_spin->setDecimals(1);
    m_y_max_spin->setEnabled(false);
    axis_grid->addWidget(m_y_max_spin, 1, 3);

    m_reset_btn = new QPushButton("Reset");
    m_reset_btn->setEnabled(false);
    axis_grid->addWidget(m_reset_btn, 1, 4);

    axis_grid->setColumnStretch(5, 1);

    main_layout->addLayout(axis_grid);

    main_layout->addSpacing(8);

    // --- Legend tree panel ---
    m_legend_panel = new QWidget;
    m_legend_panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_legend_layout = new QVBoxLayout(m_legend_panel);
    m_legend_layout->setContentsMargins(0, 0, 0, 0);
    m_legend_layout->setSpacing(2);
    main_layout->addWidget(m_legend_panel);
}

void PlotWidget::setUpConnections()
{
    connect(m_title_edit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_updating_from_vm && m_view_model)
            m_view_model->setPlotTitle(m_title_edit->text());
    });

    connect(m_y_min_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PlotWidget::onManualYChanged);
    connect(m_y_max_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PlotWidget::onManualYChanged);

    connect(m_x_start_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PlotWidget::onXRangeChanged);
    connect(m_x_stop_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &PlotWidget::onXRangeChanged);

    connect(m_reset_btn, &QPushButton::clicked, this, &PlotWidget::onResetAxes);

    connect(m_plot->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged),
            this, [this](const QCPRange& range) { handlePlotXRangeChanged(range.lower, range.upper); });
    connect(m_plot->yAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged),
            this, [this](const QCPRange& range) { handlePlotYRangeChanged(range.lower, range.upper); });
}

void PlotWidget::rebuildLegend()
{
    if (!m_view_model) return;

    // Clear existing legend contents
    m_legend_trees.clear();
    QLayoutItem* item;
    while ((item = m_legend_layout->takeAt(0)) != nullptr)
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

    const auto& all_series = m_view_model->allSeries();
    if (all_series.isEmpty())
        return;

    // Group series by receiver index (sorted)
    QMap<int, QVector<int>> receiver_groups;
    for (int i = 0; i < all_series.size(); i++)
        receiver_groups[all_series[i].receiverIndex].append(i);

    int receiver_count = receiver_groups.size();

    // Button row: Expand All, Select All, Select None
    QHBoxLayout* btn_row = new QHBoxLayout;
    btn_row->setContentsMargins(0, 0, 0, 0);

    QPushButton* toggle_btn = new QPushButton("Expand All");
    toggle_btn->setFlat(true);
    QPushButton* select_all_btn = new QPushButton("Select All");
    select_all_btn->setFlat(true);
    QPushButton* select_none_btn = new QPushButton("Select None");
    select_none_btn->setFlat(true);

    btn_row->addWidget(toggle_btn);
    btn_row->addWidget(select_all_btn);
    btn_row->addWidget(select_none_btn);
    btn_row->addStretch(1);
    m_legend_layout->addLayout(btn_row);

    // Distribute receivers across up to 4 columns
    const int num_columns = UIConstants::kReceiverGridColumns;
    int actual_columns = qMin(num_columns, receiver_count);
    int per_column = (receiver_count + actual_columns - 1) / actual_columns;

    QHBoxLayout* columns_layout = new QHBoxLayout;
    columns_layout->setSpacing(2);
    columns_layout->setContentsMargins(0, 0, 0, 0);

    // Build ordered list of receiver keys
    QVector<int> receiver_keys;
    for (auto it = receiver_groups.constBegin(); it != receiver_groups.constEnd(); ++it)
        receiver_keys.append(it.key());

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
        tree->setFixedHeight(per_column * UIConstants::kTreeItemHeightFactor +
                             UIConstants::kTreeHeightBuffer);
        tree->setStyleSheet("QTreeWidget { background: transparent; }");
        tree->setAttribute(Qt::WA_TranslucentBackground);

        for (int r = start; r < end; r++)
        {
            int receiver_num = receiver_keys[r];
            const QVector<int>& indices = receiver_groups[receiver_num];

            QTreeWidgetItem* receiver_item = new QTreeWidgetItem;
            receiver_item->setText(0, "RCVR " + QString::number(receiver_num));
            receiver_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
            receiver_item->setData(0, Qt::UserRole, -1);

            // Color receiver title to match its first channel's plot color
            if (!indices.isEmpty())
                receiver_item->setForeground(0, all_series[indices.first()].color);

            for (int idx : indices)
            {
                const PlotSeriesData& s = all_series[idx];
                QTreeWidgetItem* channel_item = new QTreeWidgetItem;
                channel_item->setText(0, s.name);
                channel_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                channel_item->setCheckState(0, s.visible ? Qt::Checked : Qt::Unchecked);
                channel_item->setForeground(0, s.color);
                channel_item->setData(0, Qt::UserRole, idx);
                receiver_item->addChild(channel_item);
            }

            tree->addTopLevelItem(receiver_item);
        }

        tree->collapseAll();

        // Connect item changes to visibility toggle
        connect(tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
            if (m_updating_from_vm || !m_view_model || column != 0) return;

            if (item->parent())
            {
                // Leaf node: toggle single series
                int idx = item->data(0, Qt::UserRole).toInt();
                bool checked = item->checkState(0) == Qt::Checked;
                onLegendCheckboxToggled(idx, checked);
            }
        });

        columns_layout->addWidget(tree);
        m_legend_trees.append(tree);
    }

    columns_layout->addStretch(1);
    m_legend_layout->addLayout(columns_layout);

    // Sync scrollbars across columns
    if (m_legend_trees.size() > 1)
    {
        QScrollBar* visible_bar = m_legend_trees.last()->verticalScrollBar();
        for (int i = 0; i < m_legend_trees.size() - 1; i++)
        {
            m_legend_trees[i]->verticalScrollBar()->setFixedWidth(0);
            connect(visible_bar, &QScrollBar::valueChanged,
                    m_legend_trees[i]->verticalScrollBar(), &QScrollBar::setValue);
        }
    }

    // Expand All / Collapse All toggle
    connect(toggle_btn, &QPushButton::clicked, this, [this, toggle_btn]() {
        bool any_collapsed = false;
        for (QTreeWidget* t : m_legend_trees)
            for (int i = 0; i < t->topLevelItemCount(); i++)
                if (!t->topLevelItem(i)->isExpanded())
                    any_collapsed = true;

        for (QTreeWidget* t : m_legend_trees)
        {
            if (any_collapsed)
                t->expandAll();
            else
                t->collapseAll();
        }
        toggle_btn->setText(any_collapsed ? "Collapse All" : "Expand All");
    });

    // Select All: check all channels
    connect(select_all_btn, &QPushButton::clicked, this, [this]() {
        if (!m_view_model) return;
        m_updating_from_vm = true;
        for (QTreeWidget* t : m_legend_trees)
        {
            t->blockSignals(true);
            for (int r = 0; r < t->topLevelItemCount(); r++)
            {
                QTreeWidgetItem* rcvr = t->topLevelItem(r);
                for (int c = 0; c < rcvr->childCount(); c++)
                {
                    rcvr->child(c)->setCheckState(0, Qt::Checked);
                    int idx = rcvr->child(c)->data(0, Qt::UserRole).toInt();
                    m_view_model->setSeriesVisible(idx, true);
                }
            }
            t->blockSignals(false);
        }
        m_updating_from_vm = false;
        rebuildChart();
    });

    // Select None: uncheck all channels
    connect(select_none_btn, &QPushButton::clicked, this, [this]() {
        if (!m_view_model) return;
        m_updating_from_vm = true;
        for (QTreeWidget* t : m_legend_trees)
        {
            t->blockSignals(true);
            for (int r = 0; r < t->topLevelItemCount(); r++)
            {
                QTreeWidgetItem* rcvr = t->topLevelItem(r);
                for (int c = 0; c < rcvr->childCount(); c++)
                {
                    rcvr->child(c)->setCheckState(0, Qt::Unchecked);
                    int idx = rcvr->child(c)->data(0, Qt::UserRole).toInt();
                    m_view_model->setSeriesVisible(idx, false);
                }
            }
            t->blockSignals(false);
        }
        m_updating_from_vm = false;
        rebuildChart();
    });
}
