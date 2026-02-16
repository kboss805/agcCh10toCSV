/**
 * @file tst_plotviewmodel.cpp
 * @brief Implementation of PlotViewModel unit tests.
 */

#include "tst_plotviewmodel.h"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTextStream>
#include <QtTest>

#include "constants.h"
#include "plotviewmodel.h"

/// Helper: writes CSV content to a temp file and returns its path.
/// The caller is responsible for deleting the file.
static QString writeTempCsv(const QString& content)
{
    QTemporaryFile file;
    file.setAutoRemove(false);
    file.setFileTemplate(QDir::tempPath() + "/tst_plot_XXXXXX.csv");
    if (!file.open())
        return {};
    QTextStream stream(&file);
    stream << content;
    file.close();
    return file.fileName();
}

void TestPlotViewModel::defaultState()
{
    PlotViewModel vm;
    QVERIFY(!vm.hasData());
    QCOMPARE(vm.seriesCount(), 0);
    QCOMPARE(vm.plotTitle(), QString(PlotConstants::kDefaultPlotTitle));
    QVERIFY(vm.yAutoScale());
}

void TestPlotViewModel::loadCsvFile()
{
    QString csv =
        "Day,Time,L_RCVR1,R_RCVR1,L_RCVR2\n"
        "45,10:00:00.000,-80.5,-75.2,-90.1\n"
        "45,10:00:01.000,-80.3,-75.0,-89.8\n"
        "45,10:00:02.000,-80.1,-74.8,-89.5\n";
    QString path = writeTempCsv(csv);
    QVERIFY(!path.isEmpty());

    PlotViewModel vm;
    QSignalSpy spy(&vm, &PlotViewModel::dataChanged);

    QVERIFY(vm.loadCsvFile(path));
    QCOMPARE(spy.count(), 1);
    QVERIFY(vm.hasData());
    QCOMPARE(vm.seriesCount(), 3);

    // Verify series names
    QCOMPARE(vm.seriesAt(0).name, QString("L_RCVR1"));
    QCOMPARE(vm.seriesAt(1).name, QString("R_RCVR1"));
    QCOMPARE(vm.seriesAt(2).name, QString("L_RCVR2"));

    // Verify data point count
    QCOMPARE(vm.seriesAt(0).xValues.size(), 3);
    QCOMPARE(vm.seriesAt(0).yValues.size(), 3);

    // Verify first Y value
    QCOMPARE(vm.seriesAt(0).yValues[0], -80.5);

    QFile::remove(path);
}

void TestPlotViewModel::csvTimeConversion()
{
    QString csv =
        "Day,Time,L_RCVR1\n"
        "45,10:00:00.000,-80.0\n"
        "45,10:00:05.500,-79.0\n"
        "46,10:00:00.000,-78.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    // First sample: elapsed = 0.0
    QCOMPARE(vm.seriesAt(0).xValues[0], 0.0);
    // Second sample: 5.5 seconds later
    QCOMPARE(vm.seriesAt(0).xValues[1], 5.5);
    // Third sample: next day same time = 86400.0 seconds later
    QCOMPARE(vm.seriesAt(0).xValues[2], 86400.0);

    // X range should span from 0 to 86400.0
    QCOMPARE(vm.xMin(), 0.0);
    QCOMPARE(vm.xMax(), 86400.0);

    QFile::remove(path);
}

void TestPlotViewModel::seriesColorAssignment()
{
    QString csv =
        "Day,Time,L_RCVR1,R_RCVR1,L_RCVR2\n"
        "1,00:00:00.000,-80.0,-75.0,-90.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    // Same receiver (RCVR1) channels should share hue
    QColor c0 = vm.seriesAt(0).color; // L_RCVR1
    QColor c1 = vm.seriesAt(1).color; // R_RCVR1
    QColor c2 = vm.seriesAt(2).color; // L_RCVR2

    // RCVR1 channels share base hue
    QCOMPARE(c0.hue(), c1.hue());
    // RCVR2 has a different hue
    QVERIFY(c0.hue() != c2.hue());

    // Second channel of same receiver has lower saturation
    QVERIFY(c1.saturation() < c0.saturation());

    QFile::remove(path);
}

void TestPlotViewModel::yAutoRange()
{
    QString csv =
        "Day,Time,L_RCVR1\n"
        "1,00:00:00.000,12.3\n"
        "1,00:00:01.000,47.8\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    // Rounded to nearest 5 dB, clipped at 0: floor(12.3/5)*5=10, ceil(47.8/5)*5=50
    double expected_min = 10.0;
    double expected_max = 50.0;
    QCOMPARE(vm.yMin(), expected_min);
    QCOMPARE(vm.yMax(), expected_max);
    QCOMPARE(vm.dataYMin(), expected_min);
    QCOMPARE(vm.dataYMax(), expected_max);

    QFile::remove(path);
}

void TestPlotViewModel::yManualRange()
{
    QString csv =
        "Day,Time,L_RCVR1\n"
        "1,00:00:00.000,-100.0\n"
        "1,00:00:01.000,-50.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    QSignalSpy spy(&vm, &PlotViewModel::axisRangeChanged);

    // Set manual range
    vm.setYManualRange(-120.0, -30.0);
    QVERIFY(!vm.yAutoScale());
    QCOMPARE(vm.yMin(), -120.0);
    QCOMPARE(vm.yMax(), -30.0);
    QCOMPARE(spy.count(), 1);

    // Reset to auto
    vm.resetYRange();
    QVERIFY(vm.yAutoScale());
    // Should revert to auto-computed range
    QVERIFY(vm.yMin() > -120.0);
    QCOMPARE(spy.count(), 2);

    QFile::remove(path);
}

void TestPlotViewModel::xTimeWindow()
{
    QString csv =
        "Day,Time,L_RCVR1\n"
        "1,00:00:00.000,-80.0\n"
        "1,00:01:00.000,-75.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    QSignalSpy spy(&vm, &PlotViewModel::axisRangeChanged);

    // Set X view to 10-30 seconds
    vm.setXViewRange(10.0, 30.0);
    QCOMPARE(vm.xViewMin(), 10.0);
    QCOMPARE(vm.xViewMax(), 30.0);
    QCOMPARE(spy.count(), 1);

    // Reset
    vm.resetXRange();
    QCOMPARE(vm.xViewMin(), vm.xMin());
    QCOMPARE(vm.xViewMax(), vm.xMax());
    QCOMPARE(spy.count(), 2);

    QFile::remove(path);
}

void TestPlotViewModel::seriesVisibility()
{
    QString csv =
        "Day,Time,L_RCVR1,R_RCVR1\n"
        "1,00:00:00.000,-80.0,-75.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));

    QSignalSpy spy(&vm, &PlotViewModel::seriesVisibilityChanged);

    // Both visible by default
    QVERIFY(vm.seriesAt(0).visible);
    QVERIFY(vm.seriesAt(1).visible);

    // Hide first series
    vm.setSeriesVisible(0, false);
    QVERIFY(!vm.seriesAt(0).visible);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);

    // Out-of-bounds is a no-op
    vm.setSeriesVisible(-1, false);
    vm.setSeriesVisible(99, false);
    QCOMPARE(spy.count(), 1);

    QFile::remove(path);
}

void TestPlotViewModel::clearData()
{
    QString csv =
        "Day,Time,L_RCVR1\n"
        "1,00:00:00.000,-80.0\n";
    QString path = writeTempCsv(csv);

    PlotViewModel vm;
    QVERIFY(vm.loadCsvFile(path));
    QVERIFY(vm.hasData());

    QSignalSpy spy(&vm, &PlotViewModel::dataChanged);
    vm.clearData();

    QVERIFY(!vm.hasData());
    QCOMPARE(vm.seriesCount(), 0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(vm.plotTitle(), QString(PlotConstants::kDefaultPlotTitle));

    QFile::remove(path);
}

void TestPlotViewModel::plotTitleDefault()
{
    PlotViewModel vm;
    QCOMPARE(vm.plotTitle(), QString(PlotConstants::kDefaultPlotTitle));
}

void TestPlotViewModel::plotTitleChange()
{
    PlotViewModel vm;
    QSignalSpy spy(&vm, &PlotViewModel::plotTitleChanged);

    vm.setPlotTitle("My Custom Title");
    QCOMPARE(vm.plotTitle(), QString("My Custom Title"));
    QCOMPARE(spy.count(), 1);

    // Setting same value is a no-op
    vm.setPlotTitle("My Custom Title");
    QCOMPARE(spy.count(), 1);
}

void TestPlotViewModel::loadInvalidFile()
{
    PlotViewModel vm;
    QVERIFY(!vm.loadCsvFile("/nonexistent/path.csv"));
    QVERIFY(!vm.hasData());
}

void TestPlotViewModel::loadEmptyFile()
{
    QString path = writeTempCsv("");
    PlotViewModel vm;
    QVERIFY(!vm.loadCsvFile(path));
    QVERIFY(!vm.hasData());
    QFile::remove(path);
}
