/**
 * @file receivergridwidget.h
 * @brief Self-contained multi-column tree grid for receiver/channel selection.
 */

#ifndef RECEIVERGRIDWIDGET_H
#define RECEIVERGRIDWIDGET_H

#include <functional>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVector>

#include "constants.h"

/**
 * @brief Multi-column tree grid for selecting receiver/channel combinations.
 *
 * Manages expand/collapse toggle, tri-state checkboxes, and synchronized
 * scrollbars across columns. Emits receiverChecked() when the user toggles
 * a channel checkbox.
 */
class ReceiverGridWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit ReceiverGridWidget(QWidget* parent = nullptr);

    /**
     * @brief Rebuilds the entire tree grid for the given receiver/channel layout.
     * @param[in] receiver_count       Number of receivers to display.
     * @param[in] channels_per_receiver Number of channels per receiver.
     * @param[in] channel_prefix_fn    Callback returning the prefix for channel index.
     * @param[in] checked_fn           Callback returning initial checked state.
     */
    void rebuild(int receiver_count, int channels_per_receiver,
                 const std::function<QString(int)>& channel_prefix_fn,
                 const std::function<bool(int, int)>& checked_fn);

    /// Updates a single receiver/channel checkbox without triggering signals.
    void setReceiverChecked(int receiver_index, int channel_index, bool checked);

    /// Enables or disables all tree widgets.
    void setAllEnabled(bool enabled);

    /// Checks or unchecks all receiver channel checkboxes.
    void setAllChecked(bool checked);

signals:
    /// Emitted when the user toggles a channel checkbox.
    void receiverChecked(int receiver_index, int channel_index, bool checked);
    /// Emitted when the user clicks "Select All".
    void selectAllRequested();
    /// Emitted when the user clicks "Select None".
    void selectNoneRequested();

private:
    /// Forwards tree item check-state changes, filtering to leaf (channel) items only.
    void onTreeItemChanged(QTreeWidgetItem* item, int column);

    QVBoxLayout* m_layout;                 ///< Internal vertical layout.
    QVector<QTreeWidget*> m_trees;         ///< Column tree widgets.
    bool m_updating_externally;            ///< Guard flag to prevent signal loops.
};

#endif // RECEIVERGRIDWIDGET_H
