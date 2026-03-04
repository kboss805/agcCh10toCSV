/**
 * @file tst_receivergridwidget.h
 * @brief Unit tests for ReceiverGridWidget signals and tree population.
 */

#ifndef TST_RECEIVERGRIDWIDGET_H
#define TST_RECEIVERGRIDWIDGET_H

#include <QObject>

class TestReceiverGridWidget : public QObject
{
    Q_OBJECT

private slots:
    void constructorCreatesWidget();
    void rebuildCreatesTreeItems();
    void setAllCheckedToggles();
    void selectAllSignalEmitted();
    void selectNoneSignalEmitted();
    void rebuildWithZeroReceivers();
    void rebuildWithOneReceiver();
};

#endif // TST_RECEIVERGRIDWIDGET_H
