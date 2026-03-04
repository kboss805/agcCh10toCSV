#include "tst_chapter10reader.h"

#include <QtTest>

#include <QCoreApplication>
#include <QDir>

#include "chapter10reader.h"
#include "constants.h"

static QString testDataPath(const QString& filename)
{
    // tests/data/ relative to the test executable location
    QDir dir(QCoreApplication::applicationDirPath());
    // Navigate up from debug/ or release/ build dir into tests/
    dir.cdUp();
    return dir.filePath("data/" + filename);
}

void TestChapter10Reader::loadChannelsReturnsTrueForValidFile()
{
    Chapter10Reader reader;
    bool result = reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));
    QVERIFY2(result, "loadChannels should return true for a valid .ch10 file");
}

void TestChapter10Reader::loadChannelsPopulatesTimeChannels()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    QStringList time_list = reader.getTimeChannelComboBoxList();
    QVERIFY2(!time_list.isEmpty(), "Time channel list should not be empty after loading a valid file");
}

void TestChapter10Reader::loadChannelsPopulatesPcmChannels()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    QVERIFY2(!pcm_list.isEmpty(), "PCM channel list should not be empty after loading a valid file");
}

void TestChapter10Reader::comboBoxListsContainChannelIdAndName()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    QStringList time_list = reader.getTimeChannelComboBoxList();
    QVERIFY(!time_list.isEmpty());
    // Each entry should be formatted as "ID - Name" where Name is non-empty
    for (const QString& entry : time_list)
    {
        QStringList parts = entry.split(" - ");
        QVERIFY2(parts.size() >= 2, qPrintable("Time entry missing ' - ' separator: " + entry));
        QVERIFY2(!parts.last().isEmpty(), qPrintable("Time channel has no name label: " + entry));
    }

    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    QVERIFY(!pcm_list.isEmpty());
    for (const QString& entry : pcm_list)
    {
        QStringList parts = entry.split(" - ");
        QVERIFY2(parts.size() >= 2, qPrintable("PCM entry missing ' - ' separator: " + entry));
        QVERIFY2(!parts.last().isEmpty(), qPrintable("PCM channel has no name label: " + entry));
    }
}

void TestChapter10Reader::currentTimeChannelSetAfterLoad()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    // After loading, a default time channel should be selected (not -1)
    QVERIFY2(reader.getCurrentTimeChannelID() != -1,
             "Current time channel should be set after loading a file with time channels");
}

void TestChapter10Reader::currentPcmChannelSetAfterLoad()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    // After loading, a default PCM channel should be selected (not -1)
    QVERIFY2(reader.getCurrentPCMChannelID() != -1,
             "Current PCM channel should be set after loading a file with PCM channels");
}

void TestChapter10Reader::clearSettingsResetsChannels()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    // Verify channels were loaded
    QVERIFY(!reader.getTimeChannelComboBoxList().isEmpty());
    QVERIFY(!reader.getPCMChannelComboBoxList().isEmpty());

    reader.clearSettings();

    QVERIFY2(reader.getTimeChannelComboBoxList().isEmpty(),
             "Time channel list should be empty after clearSettings");
    QVERIFY2(reader.getPCMChannelComboBoxList().isEmpty(),
             "PCM channel list should be empty after clearSettings");
    QCOMPARE(reader.getCurrentTimeChannelID(), -1);
    QCOMPARE(reader.getCurrentPCMChannelID(), -1);
}

void TestChapter10Reader::timeChannelChangedUpdatesSelection()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    QStringList time_list = reader.getTimeChannelComboBoxList();
    if (time_list.size() < 1)
        QSKIP("Not enough time channels in test file to test selection change");

    // Index 0 in combo = "Select a Time Stream" placeholder, index 1 = first real channel
    reader.timeChannelChanged(1);
    int first_id = reader.getCurrentTimeChannelID();
    QVERIFY2(first_id != -1, "Selecting first time channel should set a valid ID");
}

void TestChapter10Reader::pcmChannelChangedUpdatesSelection()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    if (pcm_list.size() < 1)
        QSKIP("Not enough PCM channels in test file to test selection change");

    // Index 0 in combo = placeholder, index 1 = first real channel
    reader.pcmChannelChanged(1);
    int first_id = reader.getCurrentPCMChannelID();
    QVERIFY2(first_id != -1, "Selecting first PCM channel should set a valid ID");
}

void TestChapter10Reader::loadChannelsReturnsFalseForInvalidFile()
{
    Chapter10Reader reader;
    bool result = reader.loadChannels("nonexistent_file.ch10");
    QVERIFY2(!result, "loadChannels should return false for a nonexistent file");
}

void TestChapter10Reader::getFirstPcmChannelIdReturnsValidId()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    int first_pcm = reader.getFirstPCMChannelID();
    QVERIFY2(first_pcm != -1, "getFirstPCMChannelID should return a valid ID after loading");
    QCOMPARE(first_pcm, reader.getCurrentPCMChannelID());
}

void TestChapter10Reader::timeAccessorsReturnNonZeroAfterLoad()
{
    Chapter10Reader reader;
    reader.loadChannels(testDataPath("STEPCAL 1 (NRZ-L).ch10"));

    // At least DOY should be non-zero for a valid file
    QVERIFY2(reader.getStartDayOfYear() > 0, "Start DOY should be positive after loading");
    QVERIFY2(reader.getStopDayOfYear() > 0, "Stop DOY should be positive after loading");
}
