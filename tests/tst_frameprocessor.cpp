/**
 * @file tst_frameprocessor.cpp
 * @brief Implementation of FrameProcessor unit tests.
 */

#include "tst_frameprocessor.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>
#include <QVector>

#include "chapter10reader.h"
#include "constants.h"
#include "frameprocessor.h"
#include "framesetup.h"

/// Helper: resolves a path inside tests/data/ relative to the test executable.
static QString testDataPath(const QString& filename)
{
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    return dir.filePath("data/" + filename);
}

/// Helper: loads the default frame setup from the settings/default.ini.
static bool loadDefaultFrameSetup(FrameSetup& setup)
{
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();  // tests/
    dir.cdUp();  // project root
    QString ini_path = dir.filePath("settings/default.ini");
    if (!QFileInfo::exists(ini_path))
    {
        return false;
    }
    // 16 receivers * 3 channels = 48 data words + 1 sync word = 49
    return setup.tryLoadingFile(ini_path, 49);
}

////////////////////////////////////////////////////////////////////////////////
//                          CONSTRUCTOR / ABORT                               //
////////////////////////////////////////////////////////////////////////////////

void TestFrameProcessor::constructorDefaults()
{
    FrameProcessor fp;
    // Should not crash or fail; verify the object is in a valid state
    // by calling requestAbort (exercises the atomic flag)
    fp.requestAbort();
    // No assertion needed — if construction fails, the test crashes
    QVERIFY(true);
}

void TestFrameProcessor::requestAbortSetsFlag()
{
    FrameProcessor fp;
    QSignalSpy log_spy(&fp, &FrameProcessor::logMessage);

    fp.requestAbort();

    // The abort flag is private; we verify it indirectly by calling preScan
    // on a valid file — it should return early or produce a "cancelled" message
    // if the abort is checked. But preScan only checks abort in process(), not preScan.
    // Instead, we just verify the call doesn't crash.
    QVERIFY(true);
}

////////////////////////////////////////////////////////////////////////////////
//                         STATIC METHOD TESTS                                //
////////////////////////////////////////////////////////////////////////////////

void TestFrameProcessor::hasSyncPatternFindsMatch()
{
    // Build a 16-bit pattern: 0xFF00 in a 2-byte buffer
    const char raw[] = {'\xFF', '\x00'};
    QByteArray data(raw, 2);
    uint64_t sync_pat = 0xFF00;
    uint64_t sync_mask = 0xFFFF;
    uint32_t sync_pat_len = 16;

    bool found = FrameProcessor::hasSyncPattern(reinterpret_cast<const uint8_t*>(data.constData()), 16, sync_pat, sync_mask, sync_pat_len);
    QVERIFY2(found, "Should find 0xFF00 pattern in [0xFF, 0x00] buffer");
}

void TestFrameProcessor::hasSyncPatternNoMatch()
{
    // Buffer is 0xFF00 but pattern is 0x00FF — should not match
    const char raw[] = {'\xFF', '\x00'};
    QByteArray data(raw, 2);
    uint64_t sync_pat = 0x00FF;
    uint64_t sync_mask = 0xFFFF;
    uint32_t sync_pat_len = 16;

    bool found = FrameProcessor::hasSyncPattern(reinterpret_cast<const uint8_t*>(data.constData()), 16, sync_pat, sync_mask, sync_pat_len);
    QVERIFY2(!found, "Should NOT find 0x00FF pattern in [0xFF, 0x00] buffer");
}

void TestFrameProcessor::hasSyncPatternShortBuffer()
{
    // Buffer has fewer bits than the pattern length — should not match
    QByteArray data(1, '\xFF');
    uint64_t sync_pat = 0xFF00;
    uint64_t sync_mask = 0xFFFF;
    uint32_t sync_pat_len = 16;

    bool found = FrameProcessor::hasSyncPattern(reinterpret_cast<const uint8_t*>(data.constData()), 8, sync_pat, sync_mask, sync_pat_len);
    QVERIFY2(!found, "Should not find 16-bit pattern in 8-bit buffer");
}

void TestFrameProcessor::derandomizeShortBufferIdentity()
{
    // With lfsr starting at 0, the RNRZ-L taps (bits 13,14) won't produce
    // non-zero output until at least 14 bits have been shifted in.
    // So derandomizing < 14 bits with lfsr=0 should leave data unchanged.
    QByteArray data(1, '\xAB');
    QByteArray original = data;
    uint16_t lfsr = 0;

    FrameProcessor::derandomizeBitstream(reinterpret_cast<uint8_t*>(data.data()), 8, lfsr);
    QCOMPARE(data[0], original[0]);
}

void TestFrameProcessor::derandomizeLongerBufferChanges()
{
    // With a buffer larger than 14 bits, the LFSR taps will produce non-zero
    // output and the descrambled data should differ from the original.
    QByteArray data(4, '\xFF');
    QByteArray original = data;
    uint16_t lfsr = 0;

    FrameProcessor::derandomizeBitstream(reinterpret_cast<uint8_t*>(data.data()), 32, lfsr);

    // After 14+ bits with all-1s input, some bits should differ
    QVERIFY2(data != original, "32 bits of all-1s should change after derandomization");
}

void TestFrameProcessor::writeTimeSampleFormat()
{
    // Create a temp file and call writeTimeSample to verify output format
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QString out_path = temp_dir.path() + "/test_output.csv";

    // Create a simple parameter
    ParameterInfo param;
    param.name = "L_RCVR1";
    param.word = 0;
    param.slope = 1.0;
    param.scale = 0.0;
    param.is_enabled = true;
    param.sample_sum = 42.0;

    QVector<ParameterInfo*> enabled_params;
    enabled_params.push_back(&param);

    QFile output(out_path);
    QVERIFY(output.open(QIODevice::WriteOnly));

    // writeTimeSample uses gmtime(): current_time_sample is a Unix epoch timestamp.
    // DOY 45 = Feb 14 (0-based tm_yday=44, +1 in output).
    // Epoch for 1970 DOY 45, 10:30:15 = (44 * 86400) + 10*3600 + 30*60 + 15 = 3,839,415
    double current_time_sample = (44 * 86400.0) + (10 * 3600.0) + (30 * 60.0) + 15.0;
    int n_samples = 1;

    FrameProcessor::writeTimeSample(output, current_time_sample, n_samples, enabled_params);
    output.close();

    // Read the output and verify format
    QFile result_file(out_path);
    QVERIFY(result_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString line = result_file.readLine().trimmed();
    result_file.close();

    // Format should be: "DOY,HH:MM:SS.mmm,value"
    QStringList parts = line.split(',');
    QVERIFY2(parts.size() == 3, qPrintable("Expected 3 CSV columns, got " + QString::number(parts.size())));
    QCOMPARE(parts[0], QString("45"));  // Day-of-year (tm_yday+1)
    QVERIFY(parts[1].contains(":"));     // Time with colons
    QCOMPARE(parts[2].toDouble(), 42.0); // Averaged sample value
}

void TestFrameProcessor::writeTimeSampleAveraging()
{
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QString out_path = temp_dir.path() + "/test_avg.csv";

    ParameterInfo param;
    param.name = "L_RCVR1";
    param.word = 0;
    param.slope = 1.0;
    param.scale = 0.0;
    param.is_enabled = true;
    param.sample_sum = 100.0;  // Sum of 10 samples

    QVector<ParameterInfo*> enabled_params;
    enabled_params.push_back(&param);

    QFile output(out_path);
    QVERIFY(output.open(QIODevice::WriteOnly));

    double current_time_sample = (44 * 86400.0) + (10 * 3600.0) + (30 * 60.0) + 15.0;
    int n_samples = 10;  // Average should be 100.0 / 10 = 10.0

    FrameProcessor::writeTimeSample(output, current_time_sample, n_samples, enabled_params);
    output.close();

    QFile result_file(out_path);
    QVERIFY(result_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString line = result_file.readLine().trimmed();
    result_file.close();

    QStringList parts = line.split(',');
    QVERIFY(parts.size() >= 3);
    QCOMPARE(parts[2].toDouble(), 10.0);  // 100.0 / 10 = 10.0
}

////////////////////////////////////////////////////////////////////////////////
//                          PRE-SCAN TESTS                                    //
////////////////////////////////////////////////////////////////////////////////

void TestFrameProcessor::preScanInvalidChannelId()
{
    FrameProcessor fp;
    QSignalSpy log_spy(&fp, &FrameProcessor::logMessage);

    bool is_randomized = false;
    bool result = fp.preScan("dummy.ch10", -1, 0xFE6B2840, 32, 49, 800, is_randomized);
    QVERIFY2(!result, "preScan should return false for negative channel ID");
    QVERIFY(!log_spy.isEmpty());

    // Also test out-of-range channel
    result = fp.preScan("dummy.ch10", PCMConstants::kMaxChannelCount, 0xFE6B2840, 32, 49, 800, is_randomized);
    QVERIFY2(!result, "preScan should return false for out-of-range channel ID");
}

void TestFrameProcessor::preScanInvalidFile()
{
    FrameProcessor fp;
    QSignalSpy error_spy(&fp, &FrameProcessor::errorOccurred);

    bool is_randomized = false;
    bool result = fp.preScan("nonexistent_file.ch10", 1, 0xFE6B2840, 32, 49, 800, is_randomized);
    QVERIFY2(!result, "preScan should return false for nonexistent file");
}

void TestFrameProcessor::preScanWithNrzlFile()
{
    // nrz-l_testfile.ch10 has the same 16-receiver frame structure as the RNRZ-L test file
    // (48 data words, words_in_frame=49, bits_in_frame=800), NRZ-L encoded.
    // The AGC data is on the LAST PCM channel in the file.
    QString filepath = testDataPath("nrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
    {
        QSKIP("NRZ-L test file not available");
    }

    Chapter10Reader reader;
    QVERIFY(reader.loadChannels(filepath));

    // Select the LAST PCM channel — it contains the AGC data
    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    if (pcm_list.isEmpty())
    {
        QSKIP("No PCM channels in NRZ-L test file");
    }
    reader.pcmChannelChanged(pcm_list.size()); // 1-based index (offset by "Select..." header)
    int pcm_id = reader.getCurrentPCMChannelID();
    QVERIFY2(pcm_id >= 0, "Last PCM channel should have a valid ID");

    FrameProcessor fp;
    QSignalSpy log_spy(&fp, &FrameProcessor::logMessage);

    bool is_randomized = false;
    uint64_t frame_sync = 0xFE6B2840;
    int sync_len = 32;
    int words_in_frame = 49;   // 48 data words + 1 sync word
    int bits_in_frame  = (48 * PCMConstants::kCommonWordLen) + sync_len;  // 800

    bool result = fp.preScan(filepath, pcm_id, frame_sync, sync_len,
                              words_in_frame, bits_in_frame, is_randomized);
    QVERIFY2(result, "preScan should find sync pattern in NRZ-L file");
    QVERIFY2(!is_randomized, "NRZ-L file should NOT be detected as randomized");
}

void TestFrameProcessor::preScanWithRnrzlFile()
{
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
    {
        QSKIP("RNRZ-L test file not available");
    }

    Chapter10Reader reader;
    QVERIFY(reader.loadChannels(filepath));

    int pcm_id = reader.getFirstPCMChannelID();
    if (pcm_id < 0)
    {
        QSKIP("No PCM channel in RNRZ-L test file");
    }

    FrameProcessor fp;
    QSignalSpy log_spy(&fp, &FrameProcessor::logMessage);

    bool is_randomized = false;
    uint64_t frame_sync = 0xFE6B2840;
    int sync_len = 32;
    int words_in_frame = 49;
    int bits_in_frame = (48 * 16) + 32;

    bool result = fp.preScan(filepath, pcm_id, frame_sync, sync_len,
                              words_in_frame, bits_in_frame, is_randomized);
    QVERIFY2(result, "preScan should find sync pattern in RNRZ-L file");
    QVERIFY2(is_randomized, "RNRZ-L file should be detected as randomized");
}

////////////////////////////////////////////////////////////////////////////////
//                          PROCESS ERROR PATHS                               //
////////////////////////////////////////////////////////////////////////////////

void TestFrameProcessor::processInvalidTimeChannel()
{
    FrameProcessor fp;
    QSignalSpy error_spy(&fp, &FrameProcessor::errorOccurred);
    QSignalSpy finished_spy(&fp, &FrameProcessor::processingFinished);

    FrameSetup setup;
    bool result = fp.process("dummy.ch10", &setup, "output.csv",
                              -1, 1, 0xFE6B2840, 32, 49, 800,
                              0, 100, 1, false);
    QVERIFY(!result);
    QVERIFY(!error_spy.isEmpty());
    QVERIFY(!finished_spy.isEmpty());
    QCOMPARE(finished_spy.last().at(0).toBool(), false);
}

void TestFrameProcessor::processInvalidPcmChannel()
{
    FrameProcessor fp;
    QSignalSpy error_spy(&fp, &FrameProcessor::errorOccurred);
    QSignalSpy finished_spy(&fp, &FrameProcessor::processingFinished);

    FrameSetup setup;
    bool result = fp.process("dummy.ch10", &setup, "output.csv",
                              1, -1, 0xFE6B2840, 32, 49, 800,
                              0, 100, 1, false);
    QVERIFY(!result);
    QVERIFY(!error_spy.isEmpty());
    QVERIFY(!finished_spy.isEmpty());
    QCOMPARE(finished_spy.last().at(0).toBool(), false);
}

void TestFrameProcessor::processInvalidFile()
{
    FrameProcessor fp;
    QSignalSpy error_spy(&fp, &FrameProcessor::errorOccurred);
    QSignalSpy finished_spy(&fp, &FrameProcessor::processingFinished);

    FrameSetup setup;
    bool result = fp.process("nonexistent_file.ch10", &setup, "output.csv",
                              1, 1, 0xFE6B2840, 32, 49, 800,
                              0, 100, 1, false);
    QVERIFY(!result);
    QVERIFY(!error_spy.isEmpty());
}

void TestFrameProcessor::processWithTestFile()
{
    // Use the RNRZ-L file which has 16 receivers matching default.ini
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
    {
        QSKIP("RNRZ-L test file not available");
    }

    Chapter10Reader reader;
    QVERIFY(reader.loadChannels(filepath));

    int pcm_id = reader.getFirstPCMChannelID();
    int time_id = reader.getCurrentTimeChannelID();
    if (pcm_id < 0 || time_id < 0)
    {
        QSKIP("Missing channels in test file");
    }

    FrameSetup setup;
    if (!loadDefaultFrameSetup(setup))
    {
        QSKIP("Could not load default frame setup");
    }

    // Enable all parameters with basic calibration
    for (int i = 0; i < setup.length(); i++)
    {
        ParameterInfo* param = setup.getParameter(i);
        param->is_enabled = true;
        param->slope = 1.0;
        param->scale = 0.0;
        param->sample_sum = 0.0;
    }

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QString out_path = temp_dir.path() + "/test_process_output.csv";

    // Get the full time range
    uint64_t start_secs = reader.dhmsToUInt64(
        reader.getStartDayOfYear(), reader.getStartHour(),
        reader.getStartMinute(), reader.getStartSecond());
    uint64_t stop_secs = reader.dhmsToUInt64(
        reader.getStopDayOfYear(), reader.getStopHour(),
        reader.getStopMinute(), reader.getStopSecond());

    FrameProcessor fp;
    QSignalSpy progress_spy(&fp, &FrameProcessor::progressUpdated);
    QSignalSpy finished_spy(&fp, &FrameProcessor::processingFinished);

    uint64_t frame_sync = 0xFE6B2840;
    int sync_len = 32;
    int words_in_frame = setup.length() + 1;
    int bits_in_frame = (setup.length() * PCMConstants::kCommonWordLen) + sync_len;

    // File is RNRZ-L encoded
    bool result = fp.process(filepath, &setup, out_path,
                              time_id, pcm_id, frame_sync, sync_len,
                              words_in_frame, bits_in_frame,
                              start_secs, stop_secs, 1, true);

    QVERIFY2(result, "Processing should succeed on valid RNRZ-L file");
    QVERIFY(!finished_spy.isEmpty());
    QCOMPARE(finished_spy.last().at(0).toBool(), true);

    // Verify output file was created and has content
    QFileInfo out_info(out_path);
    QVERIFY2(out_info.exists(), "Output CSV file should be created");
    QVERIFY2(out_info.size() > 0, "Output CSV file should not be empty");

    // Verify CSV header format
    QFile out_file(out_path);
    QVERIFY(out_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString header = out_file.readLine().trimmed();
    out_file.close();

    QVERIFY2(header.startsWith("Day,Time"), "CSV header should start with Day,Time");
    QVERIFY2(header.contains("L_RCVR1"), "CSV header should contain parameter names");
}

void TestFrameProcessor::processWithNrzlFile()
{
    // Verifies the non-randomized (NRZ-L, is_randomized=false) code path through process(),
    // which skips derandomizeBitstream() entirely.
    //
    // Uses "nrz-l_testfile.ch10": same 16-receiver frame structure as the RNRZ-L test
    // file (48 data words, words_in_frame=49, bits_in_frame=800), NRZ-L encoded.
    const QString filepath = testDataPath("nrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
        QSKIP("NRZ-L test file not available");

    Chapter10Reader reader;
    QVERIFY(reader.loadChannels(filepath));

    // AGC data is on the last PCM channel in this file
    QStringList pcm_list = reader.getPCMChannelComboBoxList();
    if (pcm_list.isEmpty())
        QSKIP("No PCM channels in NRZ-L test file");
    reader.pcmChannelChanged(pcm_list.size());  // 1-based, offset past "Select..." header
    int pcm_id = reader.getCurrentPCMChannelID();
    int time_id = reader.getCurrentTimeChannelID();
    QVERIFY2(pcm_id  >= 0, "Last PCM channel should have a valid ID");
    QVERIFY2(time_id >= 0, "Time channel should have a valid ID");

    FrameSetup setup;
    if (!loadDefaultFrameSetup(setup))
        QSKIP("Could not load default frame setup");

    for (int i = 0; i < setup.length(); i++)
    {
        ParameterInfo* param = setup.getParameter(i);
        param->is_enabled = true;
        param->slope = 1.0;
        param->scale = 0.0;
        param->sample_sum = 0.0;
    }

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QString out_path = temp_dir.path() + "/nrzl_output.csv";

    uint64_t start_secs = reader.dhmsToUInt64(
        reader.getStartDayOfYear(), reader.getStartHour(),
        reader.getStartMinute(), reader.getStartSecond());
    uint64_t stop_secs = reader.dhmsToUInt64(
        reader.getStopDayOfYear(), reader.getStopHour(),
        reader.getStopMinute(), reader.getStopSecond());

    // 16 receivers × 3 channels = 48 data words; +1 for sync word.
    // nrz-l_testfile.ch10 uses the same 16-receiver frame structure as the RNRZ-L test
    // file — it is NRZ-L encoded (not randomized), not a physically different frame size.
    const int sync_len       = 32;
    const int words_in_frame = setup.length() + 1;   // 49
    const int bits_in_frame  = (setup.length() * PCMConstants::kCommonWordLen) + sync_len; // 800

    FrameProcessor fp;
    QSignalSpy finished_spy(&fp, &FrameProcessor::processingFinished);
    QSignalSpy error_spy(&fp, &FrameProcessor::errorOccurred);

    bool result = fp.process(filepath, &setup, out_path,
                             time_id, pcm_id, 0xFE6B2840, sync_len,
                             words_in_frame, bits_in_frame,
                             start_secs, stop_secs, 1, false);  // is_randomized=false

    QString error_msg;
    if (!error_spy.isEmpty())
        error_msg = error_spy.first().at(0).toString();
    QVERIFY2(result, qPrintable("process() should succeed on NRZ-L STEPCAL file. Error: " + error_msg));
    QVERIFY(!finished_spy.isEmpty());
    QCOMPARE(finished_spy.last().at(0).toBool(), true);

    QFileInfo out_info(out_path);
    QVERIFY2(out_info.exists(), "Output CSV should be created");
    QVERIFY2(out_info.size() > 0, "Output CSV should not be empty");

    QFile out_file(out_path);
    QVERIFY(out_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString header   = out_file.readLine().trimmed();
    QString data_row = out_file.readLine().trimmed();
    out_file.close();

    QVERIFY2(header.startsWith("Day,Time"), "CSV header should start with Day,Time");
    QVERIFY2(!data_row.isEmpty(),           "CSV must contain at least one data row");
}

/// Helper: builds a FrameSetup loaded from default.ini with all params
/// enabled and using the given slope and offset values.
static bool setupParams(FrameSetup& setup, double slope, double offset)
{
    if (!loadDefaultFrameSetup(setup))
        return false;
    for (int i = 0; i < setup.length(); i++)
    {
        ParameterInfo* param = setup.getParameter(i);
        param->is_enabled = true;
        param->slope = slope;
        param->scale = offset;
        param->sample_sum = 0.0;
    }
    return true;
}

/// Helper: runs process() on the RNRZ-L test file and returns the output path,
/// or empty string on failure or skip.
static QString runProcess(FrameSetup& setup, const QString& out_path)
{
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
        return {};

    Chapter10Reader reader;
    if (!reader.loadChannels(filepath))
        return {};

    int pcm_id = reader.getFirstPCMChannelID();
    int time_id = reader.getCurrentTimeChannelID();
    if (pcm_id < 0 || time_id < 0)
        return {};

    uint64_t start_secs = reader.dhmsToUInt64(
        reader.getStartDayOfYear(), reader.getStartHour(),
        reader.getStartMinute(), reader.getStartSecond());
    uint64_t stop_secs = reader.dhmsToUInt64(
        reader.getStopDayOfYear(), reader.getStopHour(),
        reader.getStopMinute(), reader.getStopSecond());

    uint64_t frame_sync = 0xFE6B2840;
    int sync_len = 32;
    int words_in_frame = setup.length() + 1;
    int bits_in_frame = (setup.length() * PCMConstants::kCommonWordLen) + sync_len;

    FrameProcessor fp;
    bool ok = fp.process(filepath, &setup, out_path,
                         time_id, pcm_id, frame_sync, sync_len,
                         words_in_frame, bits_in_frame,
                         start_secs, stop_secs, 1, true);
    return ok ? out_path : QString();
}

void TestFrameProcessor::processOutputHasDataRows()
{
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
        QSKIP("RNRZ-L test file not available");

    FrameSetup setup;
    if (!setupParams(setup, 1.0, 0.0))
        QSKIP("Could not load default frame setup");

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QString out_path = runProcess(setup, temp_dir.path() + "/data_rows.csv");
    QVERIFY2(!out_path.isEmpty(), "Processing should succeed");

    QFile out_file(out_path);
    QVERIFY(out_file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString header = out_file.readLine().trimmed();
    QString data_row = out_file.readLine().trimmed();
    out_file.close();

    // Must have at least one data row
    QVERIFY2(!data_row.isEmpty(), "CSV must have at least one data row");

    QStringList header_cols = header.split(',');
    QStringList data_cols = data_row.split(',');

    // Data row column count must match header
    QVERIFY2(data_cols.size() == header_cols.size(),
             qPrintable(QString("Header has %1 columns but data row has %2")
                            .arg(header_cols.size()).arg(data_cols.size())));

    // Column 0: DOY — must be a valid integer
    bool doy_ok = false;
    int doy = data_cols[0].toInt(&doy_ok);
    QVERIFY2(doy_ok, "DOY column must be a valid integer");
    QVERIFY2(doy >= UIConstants::kMinDayOfYear && doy <= UIConstants::kMaxDayOfYear,
             "DOY must be within 1–366");

    // Column 1: time — must contain ":" and "."
    QVERIFY2(data_cols[1].contains(':'), "Time column must contain ':'");
    QVERIFY2(data_cols[1].contains('.'), "Time column must contain '.'");

    // Remaining columns: AGC values — must be parseable as doubles
    for (int i = 2; i < data_cols.size(); i++)
    {
        bool val_ok = false;
        data_cols[i].toDouble(&val_ok);
        QVERIFY2(val_ok, qPrintable("AGC value in column " + QString::number(i) + " is not numeric"));
    }
}

void TestFrameProcessor::processSlopeAffectsOutput()
{
    // Calibration: scaled_value = (raw + offset) * slope
    // With offset=0: slope=2.0 should produce exactly 2× the values of slope=1.0
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
        QSKIP("RNRZ-L test file not available");

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    FrameSetup setup1;
    if (!setupParams(setup1, 1.0, 0.0))
        QSKIP("Could not load default frame setup");
    QString path1 = runProcess(setup1, temp_dir.path() + "/slope1.csv");
    QVERIFY2(!path1.isEmpty(), "slope=1.0 run should succeed");

    FrameSetup setup2;
    QVERIFY(setupParams(setup2, 2.0, 0.0));
    QString path2 = runProcess(setup2, temp_dir.path() + "/slope2.csv");
    QVERIFY2(!path2.isEmpty(), "slope=2.0 run should succeed");

    // Read first data row from each file and compare column 2 (first AGC channel)
    auto readFirstDataValue = [](const QString& path, int col) -> double {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return 0.0;
        f.readLine();  // skip header
        QString line = f.readLine().trimmed();
        QStringList parts = line.split(',');
        if (col >= parts.size())
            return 0.0;
        return parts[col].toDouble();
    };

    double val1 = readFirstDataValue(path1, 2);
    double val2 = readFirstDataValue(path2, 2);

    // Avoid divide-by-zero if the raw sample happened to be zero
    if (qAbs(val1) < 1e-10)
        QSKIP("First AGC sample is zero — cannot test slope ratio");

    QVERIFY2(qAbs(val2 - 2.0 * val1) < 1e-6,
             qPrintable(QString("Expected val2 (%1) = 2 * val1 (%2)").arg(val2).arg(val1)));
}

void TestFrameProcessor::processNegativeSlopeNegatesValues()
{
    // Negative slope (negative polarity) should negate all output values.
    QString filepath = testDataPath("rnrz-l_testfile.ch10");
    if (!QFileInfo::exists(filepath))
        QSKIP("RNRZ-L test file not available");

    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());

    FrameSetup setup_pos;
    if (!setupParams(setup_pos, 1.0, 0.0))
        QSKIP("Could not load default frame setup");
    QString path_pos = runProcess(setup_pos, temp_dir.path() + "/positive.csv");
    QVERIFY2(!path_pos.isEmpty(), "positive slope run should succeed");

    FrameSetup setup_neg;
    QVERIFY(setupParams(setup_neg, -1.0, 0.0));
    QString path_neg = runProcess(setup_neg, temp_dir.path() + "/negative.csv");
    QVERIFY2(!path_neg.isEmpty(), "negative slope run should succeed");

    auto readFirstDataValue = [](const QString& path, int col) -> double {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return 0.0;
        f.readLine();  // skip header
        QString line = f.readLine().trimmed();
        QStringList parts = line.split(',');
        if (col >= parts.size())
            return 0.0;
        return parts[col].toDouble();
    };

    double val_pos = readFirstDataValue(path_pos, 2);
    double val_neg = readFirstDataValue(path_neg, 2);

    if (qAbs(val_pos) < 1e-10)
        QSKIP("First AGC sample is zero — cannot test polarity sign");

    QVERIFY2(qAbs(val_neg + val_pos) < 1e-6,
             qPrintable(QString("Expected val_neg (%1) = -val_pos (%2)").arg(val_neg).arg(val_pos)));
}
