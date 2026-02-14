/**
 * @file receivergridwidget.cpp
 * @brief Implementation of ReceiverGridWidget — multi-column receiver/channel tree grid.
 */

#include "receivergridwidget.h"

#include <QString>

ReceiverGridWidget::ReceiverGridWidget(QWidget* parent)
    : QGroupBox("Receivers", parent),
      m_updating_externally(false)
{
    m_layout = new QVBoxLayout;
    setLayout(m_layout);
}

void ReceiverGridWidget::rebuild(int receiver_count, int channels_per_receiver,
                                 const std::function<QString(int)>& channel_prefix_fn,
                                 const std::function<bool(int, int)>& checked_fn)
{
    // Clear existing contents
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr)
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
    m_trees.clear();

    if (receiver_count <= 0)
        return;

    // Expand/Collapse All button
    QPushButton* toggle_btn = new QPushButton("Expand All");
    toggle_btn->setFlat(true);
    m_layout->addWidget(toggle_btn, 0, Qt::AlignLeft);

    // Four columns of receiver trees
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
        // Fixed height fits all collapsed receivers; scrollbar appears when expanded
        tree->setFixedHeight(per_column * UIConstants::kTreeItemHeightFactor + UIConstants::kTreeHeightBuffer);

        for (int receiver_index = start; receiver_index < end; receiver_index++)
        {
            QTreeWidgetItem* receiver_item = new QTreeWidgetItem;
            receiver_item->setText(0, "RCVR " + QString::number(receiver_index + 1));
            receiver_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
            receiver_item->setData(0, Qt::UserRole, receiver_index);

            for (int channel_index = 0; channel_index < channels_per_receiver; channel_index++)
            {
                QTreeWidgetItem* channel_item = new QTreeWidgetItem;
                channel_item->setText(0, channel_prefix_fn(channel_index));
                channel_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                channel_item->setCheckState(0,
                    checked_fn(receiver_index, channel_index)
                        ? Qt::Checked : Qt::Unchecked);
                receiver_item->addChild(channel_item);
            }

            tree->addTopLevelItem(receiver_item);
        }

        tree->collapseAll();

        connect(tree, &QTreeWidget::itemChanged,
                this, &ReceiverGridWidget::onTreeItemChanged);

        columns_layout->addWidget(tree);
        m_trees.append(tree);
    }

    columns_layout->addStretch(1);
    m_layout->addLayout(columns_layout);

    // Hide scrollbar on non-last columns; sync all from the visible one
    if (m_trees.size() > 1)
    {
        QScrollBar* visible_bar = m_trees.last()->verticalScrollBar();
        for (int i = 0; i < m_trees.size() - 1; i++)
        {
            m_trees[i]->verticalScrollBar()->setFixedWidth(0);
            connect(visible_bar, &QScrollBar::valueChanged,
                    m_trees[i]->verticalScrollBar(), &QScrollBar::setValue);
        }
    }

    // Connect expand/collapse toggle
    connect(toggle_btn, &QPushButton::clicked, this, [this, toggle_btn]() {
        bool any_collapsed = false;
        for (QTreeWidget* t : m_trees)
            for (int i = 0; i < t->topLevelItemCount(); i++)
                if (!t->topLevelItem(i)->isExpanded())
                    any_collapsed = true;

        for (QTreeWidget* t : m_trees)
        {
            if (any_collapsed)
                t->expandAll();
            else
                t->collapseAll();
        }
        toggle_btn->setText(any_collapsed ? "Collapse All" : "Expand All");
    });
}

void ReceiverGridWidget::setReceiverChecked(int receiver_index, int channel_index, bool checked)
{
    // Find the tree item matching this receiver_index via UserRole data
    QTreeWidgetItem* target = nullptr;
    QTreeWidget* target_tree = nullptr;
    for (QTreeWidget* tree : m_trees)
    {
        for (int i = 0; i < tree->topLevelItemCount(); i++)
        {
            if (tree->topLevelItem(i)->data(0, Qt::UserRole).toInt() == receiver_index)
            {
                target = tree->topLevelItem(i);
                target_tree = tree;
                break;
            }
        }
        if (target)
            break;
    }

    if (!target || channel_index < 0 || channel_index >= target->childCount())
        return;

    m_updating_externally = true;
    target_tree->blockSignals(true);
    target->child(channel_index)->setCheckState(
        0, checked ? Qt::Checked : Qt::Unchecked);
    target_tree->blockSignals(false);
    m_updating_externally = false;
}

void ReceiverGridWidget::setAllEnabled(bool enabled)
{
    for (QTreeWidget* tree : m_trees)
        tree->setEnabled(enabled);
}

void ReceiverGridWidget::setAllChecked(bool checked)
{
    Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;
    for (QTreeWidget* tree : m_trees)
    {
        tree->blockSignals(true);
        for (int r = 0; r < tree->topLevelItemCount(); r++)
        {
            QTreeWidgetItem* receiver_item = tree->topLevelItem(r);
            for (int c = 0; c < receiver_item->childCount(); c++)
                receiver_item->child(c)->setCheckState(0, state);
        }
        tree->blockSignals(false);
    }
}

void ReceiverGridWidget::onTreeItemChanged(QTreeWidgetItem* item, int column)
{
    if (column != 0)
        return;
    if (m_updating_externally)
        return;

    // Only forward leaf (channel) items.
    // Parent (receiver) items have their state managed by Qt::ItemIsAutoTristate.
    QTreeWidgetItem* parent = item->parent();
    if (!parent)
        return;

    int receiver_index = parent->data(0, Qt::UserRole).toInt();
    int channel_index = parent->indexOfChild(item);
    bool is_checked = (item->checkState(0) == Qt::Checked);

    emit receiverChecked(receiver_index, channel_index, is_checked);
}
