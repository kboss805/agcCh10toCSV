#include "tst_framesetup.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryFile>
#include <QtTest>

#include "constants.h"
#include "framesetup.h"

static QString testDataPath(const QString& filename)
{
    // The test executable is built in tests/debug/ or tests/release/.
    // Navigate back to tests/data/.
    return QDir(QCoreApplication::applicationDirPath())
        .filePath("../../tests/data/" + filename);
}

void TestFrameSetup::defaultLengthIsZero()
{
    FrameSetup fs;
    QCOMPARE(fs.length(), 0);
}

void TestFrameSetup::clearParametersResetsToEmpty()
{
    FrameSetup fs;
    QString path = testDataPath("test_framesetup.ini");
    if (QFile::exists(path))
    {
        fs.tryLoadingFile(path, PCMConstants::kWordsInMinorFrame);
        QVERIFY(fs.length() > 0);
    }
    fs.clearParameters();
    QCOMPARE(fs.length(), 0);
}

void TestFrameSetup::getParameterInvalidIndexReturnsNull()
{
    FrameSetup fs;
    QVERIFY(fs.getParameter(0) == nullptr);
    QVERIFY(fs.getParameter(100) == nullptr);
}

void TestFrameSetup::getParameterNegativeIndexReturnsNull()
{
    FrameSetup fs;
    QVERIFY(fs.getParameter(-1) == nullptr);
}

void TestFrameSetup::tryLoadingFileValidFile()
{
    QString path = testDataPath("test_framesetup.ini");
    if (!QFile::exists(path))
        QSKIP("Test fixture file not found");

    FrameSetup fs;
    bool result = fs.tryLoadingFile(path, PCMConstants::kWordsInMinorFrame);

    QVERIFY(result);
    QCOMPARE(fs.length(), 3);

    // Parameters preserve file order: L, R, C
    const ParameterInfo* p0 = fs.getParameter(0);
    QVERIFY(p0 != nullptr);
    QCOMPARE(p0->name, QString("L_RCVR1"));
    QCOMPARE(p0->word, 0);  // INI Word=1, stored as 0
    QCOMPARE(p0->is_enabled, true);
    QCOMPARE(p0->slope, 0.0);
    QCOMPARE(p0->scale, 0.0);
    QCOMPARE(p0->sample_sum, 0.0);

    const ParameterInfo* p1 = fs.getParameter(1);
    QVERIFY(p1 != nullptr);
    QCOMPARE(p1->name, QString("R_RCVR1"));
    QCOMPARE(p1->word, 1);  // INI Word=2, stored as 1
    QCOMPARE(p1->is_enabled, true);

    const ParameterInfo* p2 = fs.getParameter(2);
    QVERIFY(p2 != nullptr);
    QCOMPARE(p2->name, QString("C_RCVR1"));
    QCOMPARE(p2->word, 2);  // INI Word=3, stored as 2
    QCOMPARE(p2->is_enabled, false);
}

void TestFrameSetup::tryLoadingFileMissingWordKey()
{
    QString path = testDataPath("test_framesetup_missing_word.ini");
    if (!QFile::exists(path))
        QSKIP("Test fixture file not found");

    FrameSetup fs;
    bool result = fs.tryLoadingFile(path, PCMConstants::kWordsInMinorFrame);

    QVERIFY(!result);
}

void TestFrameSetup::tryLoadingFileOutOfBoundsWord()
{
    QString path = testDataPath("test_framesetup_out_of_bounds.ini");
    if (!QFile::exists(path))
        QSKIP("Test fixture file not found");

    FrameSetup fs;
    bool result = fs.tryLoadingFile(path, PCMConstants::kWordsInMinorFrame);

    QVERIFY(!result);
}

void TestFrameSetup::tryLoadingFileWordZeroFails()
{
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open())
        QSKIP("Could not create temporary file");

    QSettings settings(tmp.fileName(), QSettings::IniFormat);
    settings.beginGroup("TestParam");
    settings.setValue("Word", 0);
    settings.endGroup();
    settings.sync();

    FrameSetup fs;
    bool result = fs.tryLoadingFile(tmp.fileName(), PCMConstants::kWordsInMinorFrame);
    QVERIFY(!result);
}

void TestFrameSetup::tryLoadingFileWordEqualsFrameSize()
{
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open())
        QSKIP("Could not create temporary file");

    QSettings settings(tmp.fileName(), QSettings::IniFormat);
    settings.beginGroup("TestParam");
    settings.setValue("Word", PCMConstants::kWordsInMinorFrame);
    settings.endGroup();
    settings.sync();

    FrameSetup fs;
    bool result = fs.tryLoadingFile(tmp.fileName(), PCMConstants::kWordsInMinorFrame);
    QVERIFY(!result);
}

void TestFrameSetup::saveToSettingsWritesCorrectData()
{
    QString path = testDataPath("test_framesetup.ini");
    if (!QFile::exists(path))
        QSKIP("Test fixture file not found");

    FrameSetup fs;
    fs.tryLoadingFile(path, PCMConstants::kWordsInMinorFrame);

    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open())
        QSKIP("Could not create temporary file");

    QSettings out_settings(tmp.fileName(), QSettings::IniFormat);
    fs.saveToSettings(out_settings);
    out_settings.sync();

    QSettings in_settings(tmp.fileName(), QSettings::IniFormat);

    in_settings.beginGroup("L_RCVR1");
    QCOMPARE(in_settings.value("Word").toInt(), 1);
    QCOMPARE(in_settings.value("Enabled").toBool(), true);
    in_settings.endGroup();

    in_settings.beginGroup("C_RCVR1");
    QCOMPARE(in_settings.value("Word").toInt(), 3);
    QCOMPARE(in_settings.value("Enabled").toBool(), false);
    in_settings.endGroup();
}
