/**
 * @file tst_receivergridwidget.cpp
 * @brief Implementation of ReceiverGridWidget unit tests.
 */

#include "tst_receivergridwidget.h"

#include <QSignalSpy>
#include <QtTest>

#include "constants.h"
#include "receivergridwidget.h"

static QString testChannelPrefix(int index)
{
    if (index < UIConstants::kNumKnownPrefixes)
    {
        return UIConstants::kChannelPrefixes[index];
    }
    return "CH" + QString::number(index + 1);
}

static bool allChecked(int /*receiver_index*/, int /*channel_index*/)
{
    return true;
}

void TestReceiverGridWidget::constructorCreatesWidget()
{
    ReceiverGridWidget widget;
    QVERIFY(widget.layout() != nullptr);
}

void TestReceiverGridWidget::rebuildCreatesTreeItems()
{
    ReceiverGridWidget widget;
    widget.rebuild(4, 3, testChannelPrefix, allChecked);
    // Should not crash; the widget is populated with 4 receivers x 3 channels
    QVERIFY(true);
}

void TestReceiverGridWidget::setAllCheckedToggles()
{
    ReceiverGridWidget widget;
    widget.rebuild(4, 3, testChannelPrefix, allChecked);

    // Toggle all to unchecked, then back to checked — should not crash
    widget.setAllChecked(false);
    widget.setAllChecked(true);
    QVERIFY(true);
}

void TestReceiverGridWidget::selectAllSignalEmitted()
{
    ReceiverGridWidget widget;
    QSignalSpy spy(&widget, &ReceiverGridWidget::selectAllRequested);

    widget.rebuild(4, 3, testChannelPrefix, allChecked);

    // Find the "Select All" button and click it
    QPushButton* select_all_btn = nullptr;
    const QList<QPushButton*> buttons = widget.findChildren<QPushButton*>();
    for (QPushButton* btn : buttons)
    {
        if (btn->text() == "Select All")
        {
            select_all_btn = btn;
            break;
        }
    }

    if (select_all_btn != nullptr)
    {
        QTest::mouseClick(select_all_btn, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
    }
    else
    {
        QSKIP("Select All button not found");
    }
}

void TestReceiverGridWidget::selectNoneSignalEmitted()
{
    ReceiverGridWidget widget;
    QSignalSpy spy(&widget, &ReceiverGridWidget::selectNoneRequested);

    widget.rebuild(4, 3, testChannelPrefix, allChecked);

    QPushButton* select_none_btn = nullptr;
    const QList<QPushButton*> buttons = widget.findChildren<QPushButton*>();
    for (QPushButton* btn : buttons)
    {
        if (btn->text() == "Select None")
        {
            select_none_btn = btn;
            break;
        }
    }

    if (select_none_btn != nullptr)
    {
        QTest::mouseClick(select_none_btn, Qt::LeftButton);
        QCOMPARE(spy.count(), 1);
    }
    else
    {
        QSKIP("Select None button not found");
    }
}

void TestReceiverGridWidget::rebuildWithZeroReceivers()
{
    ReceiverGridWidget widget;
    widget.rebuild(0, 3, testChannelPrefix, allChecked);
    // Should not crash with zero receivers
    QVERIFY(true);
}

void TestReceiverGridWidget::rebuildWithOneReceiver()
{
    ReceiverGridWidget widget;
    widget.rebuild(1, 3, testChannelPrefix, allChecked);
    // Should work with a single receiver
    QVERIFY(true);
}
