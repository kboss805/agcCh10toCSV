# Manual Test Procedures — agcCh10toCSV v3.2.0

These procedures cover functional areas that cannot be fully exercised by automated unit tests: GUI interactions, visual rendering, file system side effects, installer behavior, and end-to-end data validation.

**Test data files** (in `tests/data/`):
- `nrz-l_testfile.ch10` — 16 receivers, NRZ-L encoding, last PCM channel contains AGC data (primary NRZ-L test file)
- `STEPCAL 1 (NRZ-L).ch10` — 16 receivers, NRZ-L encoding (same content as nrz-l_testfile.ch10; retained for reference)
- `STEPCAL 2 (NRZ-L).ch10` — 8 receivers, NRZ-L encoding
- `STEP CALS (RNRZ-L, NRZ-L).ch10` — multi-channel file with both encoding types
- `rnrz-l_testfile.ch10` — 16 receivers, RNRZ-L encoding

**Build under test**: `build/release/agcCh10toCSV.exe`

---

## MT-01: Application Startup

**Purpose**: Verify the application launches cleanly and loads default settings.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Launch `agcCh10toCSV.exe` | Window opens; no crash; no error dialogs |
| 2 | Observe the log window | Startup log entries appear: default.ini values (FrameSync, Polarity, Slope, Scale, receiver/channel counts) |
| 3 | Observe the settings summary panel | Displays current Sync, Polarity, Slope, Scale, and Receivers values from default.ini |
| 4 | Observe the status bar | Shows a default/empty file summary (no file loaded) |
| 5 | Observe the Advanced section | The collapsible "▶ Receivers" and "▶ Time Controls" sections are collapsed by default; the receiver grid is not visible until expanded |

**Pass criteria**: Log shows startup info; no errors or warnings at startup.

---

## MT-02: Open Ch10 File (Single File)

**Purpose**: Verify file open via dialog and drag-and-drop.

### MT-02a: File Dialog

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Press Ctrl+O or click File > Open | File open dialog appears |
| 2 | Navigate to `tests/data/` and select `nrz-l_testfile.ch10` | Dialog closes; file loads |
| 3 | Observe status bar | Shows filename, file size, channel counts, and time range |
| 4 | Observe log window | Lists PCM and time channels, time range, current frame settings |
| 5 | Observe PCM and Time channel dropdowns in the file list | Populated with available channels from the file |
| 6 | Observe pre-scan result in log | Reports encoding (NRZ-L) and whether sync was found |

### MT-02b: Drag and Drop

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Drag `rnrz-l_testfile.ch10` from Explorer onto the application window | File loads as if opened via dialog |
| 2 | Verify status bar and log | Same as MT-02a steps 3–6 |

### MT-02c: Recent Files

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open two different .ch10 files via dialog (one after the other) | Both appear in File > Recent Files submenu |
| 2 | Click a recent file entry | File loads without dialog |
| 3 | Close and relaunch the application | Recent files list persists across sessions |
| 4 | Open 6 different files | Only the 5 most recent are retained |

**Pass criteria**: All file loading paths work; status bar and log update correctly.

---

### MT-02d: Dialog Directory Persistence

**Purpose**: Verify that each file dialog type independently remembers its last-used directory across sessions.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open a Ch10 file from `C:\TestData\Ch10\` | Directory saved for the Ch10 open dialog |
| 2 | Process a file and save output to `C:\TestData\CSV\` | Directory saved for the CSV save dialog |
| 3 | Open Settings and load an INI file from `C:\TestData\INI\` | Directory saved for the INI open dialog |
| 4 | Reopen each dialog type without navigating away | Each dialog opens to its own last-used directory, not the other dialogs' directories |
| 5 | Close and relaunch the application; reopen each dialog | All three directories are restored independently |

**Pass criteria**: All three dialog types save and restore their directories independently; no dialog inherits another's last path.

---

## MT-03: Single-File Processing (End-to-End)

**Purpose**: Verify a complete processing run from file open to CSV output.

### MT-03a: Process with Default Settings

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open `nrz-l_testfile.ch10` | File loads |
| 2 | Select the last PCM channel in the file list dropdown | Channel selected |
| 3 | Ensure "Extract All Time" is checked | Full time range will be processed |
| 4 | Click Process (or Ctrl+R) | Output file dialog appears |
| 5 | Choose output path and confirm | Progress bar begins updating |
| 6 | Wait for completion | Progress bar reaches 100%; log shows success message with clickable file link |
| 7 | Click the output file link in the log | Output file opens in default application (e.g., Excel, Notepad) |
| 8 | Open output CSV | First row contains column headers (Day, Time, L_RCVR1, R_RCVR1, C_RCVR1, …) |
| 9 | Check data rows | Each row has a DOY, time string, and numeric AGC values |

### MT-03b: Busy Cursor

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Start processing a large file | Cursor changes to hourglass/wait cursor |
| 2 | Processing completes | Cursor returns to normal arrow |

### MT-03c: Open Folder Link

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | After processing completes, find the "Open Folder" link in the log | Link is present and clickable |
| 2 | Click "Open Folder" | Windows Explorer opens to the output directory |

**Pass criteria**: CSV created; headers correct; data present; links work.

---

## MT-04: Data Accuracy Spot-Check

**Purpose**: Verify that exported AGC values are within the expected calibrated range for both NRZ-L and RNRZ-L encoded files.

### MT-04a: NRZ-L Data Accuracy

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Process `nrz-l_testfile.ch10` with default settings (Polarity: Negative, Slope: 0-5V, Scale: 20 dB/V) | CSV created |
| 2 | Open the CSV in Excel | Column headers present; values in dB range |
| 3 | Plot L_RCVR1 vs. Time in Excel | Step-cal pattern visible: signal steps through discrete levels |
| 4 | Verify values fall within the expected dB range for the configured slope/scale | No extreme outliers; no NaN or blank cells |
| 5 | Repeat with `STEPCAL 2 (NRZ-L).ch10` | Similar step-cal pattern; different step levels |

**Pass criteria**: Calibrated values are numerically correct and step-cal pattern is visible.

### MT-04b: RNRZ-L Data Accuracy (De-randomization)

**Purpose**: Verify that the RNRZ-L bitstream de-randomizer produces valid, calibrated output.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open `rnrz-l_testfile.ch10` | File loads; pre-scan log reports encoding as RNRZ-L (randomized) |
| 2 | Select the PCM channel; process with default settings to `rnrz_output.csv` | CSV created without errors |
| 3 | Open `rnrz_output.csv` in Excel | Column headers present; data rows present; no NaN or blank cells |
| 4 | Plot L_RCVR1 vs. Time | Step-cal pattern visible, same structure as NRZ-L output — confirms de-randomization is working |
| 5 | Compare row count with `nrz-l_testfile.ch10` output over the same time window | Row counts should be comparable; RNRZ-L values match NRZ-L values for the same recording |

**Pass criteria**: RNRZ-L output shows the same step-cal pattern as NRZ-L; de-randomization produces valid calibrated values with no garbled rows.

---

## MT-05: Time Window and Sample Rate

**Purpose**: Verify time-range filtering and sample rate averaging.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open `rnrz-l_testfile.ch10` | File loads; time range shown in status bar |
| 2 | In the left pane, click "▶ Time Controls" to expand the section | Start/Stop time fields and Sample Rate dropdown become visible |
| 3 | Uncheck "Extract All Time" | Start/Stop time fields become editable and show the file's full start and stop times |
| 4 | Enter a start time 10 seconds into the file, stop time 30 seconds | Fields accept valid DOY/time values |
| 5 | Enter an out-of-range start DOY (e.g., a day before or after the file's range) and press Tab or Enter | Field snaps back to the nearest valid file boundary; no error dialog |
| 6 | Re-check "Extract All Time" | Start/Stop fields revert to the file's full start and stop times (not zeros) |
| 7 | Uncheck "Extract All Time" again; enter valid start/stop times; set sample rate to 1 Hz | Processing runs |
| 8 | Check output CSV | Rows are spaced ~1 second apart; values are averaged over that window |
| 9 | Repeat with 100 Hz | Rows spaced ~10 ms apart |

**Pass criteria**: Time filtering limits output correctly; out-of-range times clamp to file bounds; re-checking Extract All Time restores the full file time window; sample rate affects row density.

---

## MT-06: Receiver / Channel Selection

**Purpose**: Verify receiver and channel enable/disable affects CSV output.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open a file and load default settings | Left pane shows the "▶ Receivers" collapsible section |
| 2 | Click "▶ Receivers" to expand the section | Receiver tree grid becomes visible; all receiver rows are collapsed |
| 3 | Click "Expand All" (button to the right of the tree columns) | All receiver rows expand to show individual channel checkboxes; button text changes to "Collapse All" |
| 4 | Click "Collapse All" | All receiver rows collapse; button text reverts to "Expand All" |
| 5 | Click "Select None" | All receiver checkboxes clear |
| 6 | Click Process | Error or warning in log: no receivers selected; processing does not start |
| 7 | Click "Select All" | All receiver checkboxes checked |
| 8 | Manually uncheck Receiver 1 (expand it first if needed) | Receiver 1 channels unchecked; parent item shows partial/unchecked state |
| 9 | Process and open output CSV | No columns for Receiver 1 in the CSV header |
| 10 | Uncheck only `C_RCVR2` (third channel of receiver 2) | Only that channel unchecked |
| 11 | Process and open output CSV | `C_RCVR2` column absent; other Receiver 2 columns present |

**Pass criteria**: Expand All/Collapse All toggles all receiver rows and updates button label; output columns exactly match the enabled receiver/channel selection.

---

## MT-07: Settings Dialog and INI File Handling

**Purpose**: Verify all settings fields can be changed, validation is enforced, a save/load round-trip preserves values, and INI edge cases are handled correctly.

### MT-07a: Dialog Open and Frame Sync Validation

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open Settings dialog | Current settings displayed; values match log startup output; OK button is enabled |
| 2 | Change Frame Sync to an invalid hex string (e.g., "GGGG1234") — do not click OK yet | Frame Sync field border turns red immediately; inline red error message appears below the field; OK button becomes disabled |
| 3 | Clear the Frame Sync field entirely | Same red border and inline error; OK button remains disabled |
| 4 | Type a valid hex value (e.g., "ABCD1234") | Red border clears; error message disappears; OK button re-enables |
| 5 | Click OK | Dialog closes; log window shows "Settings applied:" with the new FrameSync value |
| 6 | Reopen Settings; change Frame Sync to an invalid value; click Cancel | Log shows no new "Settings applied:" entry; Settings dialog on next open reflects the previously accepted value |

**Pass criteria**: Red border and error label appear immediately on invalid input without needing to click OK; OK button is disabled while any field is invalid and re-enables when all fields are valid; cancel does not modify settings.

---

### MT-07b: Parameter Fields (Polarity, Slope, Scale)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open Settings dialog; note the current Polarity, Slope, and Scale values | Values match the "Settings applied:" log entries from startup; OK button is enabled |
| 2 | Change Polarity to "Positive"; click OK | Log window appends "Settings applied: ... Polarity=Positive ..." |
| 3 | Open Settings; change Polarity back to "Negative"; click OK | Log appends "Settings applied: ... Polarity=Negative ..." |
| 4 | Open Settings; cycle through all four Slope options (±10V, ±5V, 0-10V, 0-5V) one at a time; click OK after each | Log appends a new "Settings applied:" entry after each OK reflecting the selected slope |
| 5 | Open Settings; change Scale to a valid positive number (e.g., "50"); click OK | Log appends "Settings applied: ... Scale=50 dB/V" |
| 6 | Open Settings; clear the Scale field and type "0" — do not click OK yet | Scale field border turns red immediately; inline red error appears below the field; OK button becomes disabled |
| 7 | Clear the Scale field and type "abc" | Same red border and inline error; OK button remains disabled |
| 8 | Clear the Scale field and type "-50" | Same red border and inline error; OK button remains disabled |
| 9 | Clear the Scale field and type "50" | Red border clears; error message disappears; OK button re-enables |
| 10 | Click OK | Dialog closes; log appends "Settings applied: ... Scale=50 dB/V" |

**Pass criteria**: Red border and error label appear immediately on invalid Scale input without needing to click OK; OK button is disabled for zero, negative, and non-numeric values and re-enables when a valid positive number is entered; each successful OK appends a "Settings applied:" log entry.

---

### MT-07c: Receiver Configuration

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open Settings; note current Number of Receivers and Channels per Receiver | Values match startup log entries; OK button is enabled |
| 2 | Change Number of Receivers to 4; click OK | Log appends "Settings applied: ... Receivers=4 ..."; receiver grid rebuilds showing 4 receivers |
| 3 | Open Settings; change Channels per Receiver to 1; click OK | Log appends updated entry; receiver grid rebuilds with 1 channel per receiver |
| 4 | Open Settings; change Number of Receivers to 16 and Channels per Receiver to 3; click OK | Log appends "Receivers=16, Channels=3"; grid shows 16 receivers × 3 channels |
| 5 | Open Settings; set Number of Receivers to 16 and Channels per Receiver to 4 (total = 64) — do not click OK yet | Inline red error appears below the Receivers group: "Receivers × Channels (64) exceeds the maximum of 48 words."; OK button becomes disabled |
| 6 | Change Channels per Receiver back to 3 | Error clears; OK button re-enables |
| 7 | Click OK | Log appends "Settings applied: Receivers=16, Channels=3" |
| 8 | Open Settings; set Number of Receivers to 0 — do not click OK yet | Number of Receivers field border turns red; error shows valid range (1–16); OK button disabled |
| 9 | Open Settings; set Number of Receivers to 17 — do not click OK yet | Same red border and error; OK button disabled |
| 10 | Restore Number of Receivers to a valid value (e.g., 8); click OK | Error clears; OK re-enables; log appends "Settings applied:" |

**Pass criteria**: Red border and error label appear immediately on invalid receiver/channel input; OK button is disabled for out-of-range counts and total exceeding 48; OK re-enables when all values are valid.

---

### MT-07d: Save and Load Round-Trip

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Set Polarity to Positive, Slope to ±5V, Scale to 80, Receivers to 8, Channels to 3; click OK | Log appends "Settings applied:" with all new values |
| 2 | Open Settings; click "Save As"; save as `test_settings.ini` | Log appends "Settings saved:" with all values listed |
| 3 | Change settings to different values (e.g., Polarity Negative, Slope 0-10V); click OK | Log appends new "Settings applied:" entry with changed values |
| 4 | Click "Load" and select `test_settings.ini` | Log appends "Loading settings:" followed by all loaded values (Sync, Polarity, Slope, Scale, Receivers, Channels) |
| 5 | Reopen Settings dialog | Fields match the values saved in step 1 |

**Pass criteria**: All field values survive a save/load round-trip; log messages confirm each applied and loaded value.

---

### MT-07e: Settings Applied to Processing

**Purpose**: Verify that changed settings are actually used when processing a Ch10 file, not just reflected in the UI.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open `nrz-l_testfile.ch10`; set Scale to 100 dB/V; process to `output_100.csv` | CSV created |
| 2 | Open Settings; change Scale to 50 dB/V; click OK; process the same file to `output_50.csv` | CSV created |
| 3 | Open both CSVs; compare a row with the same timestamp | All AGC values in `output_50.csv` are exactly half those in `output_100.csv` |
| 4 | Open Settings; set Polarity to Positive; click OK; process the same file to `output_positive.csv` | CSV created |
| 5 | Open Settings; set Polarity to Negative; click OK; process to `output_negative.csv` | CSV created |
| 6 | Compare `output_positive.csv` and `output_negative.csv` at the same timestamp | AGC values have opposite sign (positive polarity produces positive dB values; negative produces negative) |
| 7 | Open Settings; change Frame Sync to an incorrect but valid hex value (e.g., "FFFFFFFF"); click OK; attempt to process the file | Pre-scan runs, finds no sync; log shows "Pre-scan failed: no frame sync found. Processing aborted."; **no output file is created** |
| 8 | Restore the correct Frame Sync from default.ini; process again | Processing succeeds; output matches `output_100.csv` from step 1 |

**Pass criteria**: Scale change produces proportionally scaled output values; polarity change negates output values; incorrect frame sync aborts processing before any file is written.

---

### MT-07f: INI Parameter Count Mismatch Warning

**Purpose**: Verify that a manually edited or version-mismatched INI file produces a clear warning rather than silently using incomplete data.

**Setup**: Using a text editor, open a copy of `default.ini` and set `Count=4` and `ChannelsPerReceiver=3` in the `[Receivers]` section, but delete 2 of the `[RcvrN_ChM]` parameter sections so only 10 are present instead of the expected 12 (4 receivers × 3 channels).

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | In the application, open Settings and click Load; select the modified INI file | Settings load; log appends a dark-yellow WARNING line |
| 2 | Read the warning message in the log | Message reads: *"INI file defines 10 receiver/channel entries, but the configured 4 receivers × 3 channels/receiver requires 12. Some channel parameters may be missing or ignored."* |
| 3 | Open the Settings dialog | Receivers=4, Channels/Receiver=3 are applied; dialog opens normally |

**Pass criteria**: A dark-yellow WARNING appears immediately after loading the INI; the message clearly states the found count, the expected count, and the configured receiver/channel breakdown; the application remains stable and usable.

---

## MT-08: Theme Toggle (Dark / Light)

**Purpose**: Verify the dark/light theme switch applies consistently.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Launch in default (light) theme | All UI elements use light palette |
| 2 | Toggle to dark theme via View or toolbar | Background, text, dropdowns, spinboxes, and log window switch to dark palette |
| 3 | Open a file and process | Plot window (if shown) adopts dark chart background |
| 4 | Toggle back to light | All elements revert |
| 5 | Close and relaunch | Last-used theme is restored |

**Pass criteria**: All widgets (buttons, combos, spinboxes, QComboBox chevrons, plot) render correctly in both themes.

---

## MT-09: Plot Window (US6)

**Purpose**: Verify the AGC signal plot after processing.

### MT-09a: Basic Plot Display

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Process `rnrz-l_testfile.ch10` with all receivers enabled | Plot dock auto-opens on the right |
| 2 | Observe X axis | Shows DDD:HH:MM:SS time labels with ~10 tick marks |
| 3 | Observe Y axis | Auto-scaled to min/max of data with small margin |
| 4 | Observe legend | One checkbox per receiver-channel; each colored differently; receivers share a color hue |

### MT-09b: Interactive Controls

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Scroll mouse wheel over the chart | X axis zooms in/out (time axis) |
| 2 | Click and drag on the chart | Both axes pan; X axis clamps at 0 (cannot pan before start) |
| 3 | Enter a plot title in the title field | Title updates on chart immediately |
| 4 | Type values into the Y Min/Max fields | Y axis range updates; data stays visible |
| 5 | Type a valid DDD:HH:MM:SS value into the X Start or X Stop text field and press Enter or Tab | X axis zooms to the entered time window |
| 6 | Type a time outside the file's range (e.g., day 0 or a day past the end) and press Tab | Field snaps to the nearest file boundary; chart updates to the clamped range |
| 7 | Click Reset (button to the right of the X Stop field) | X and Y axes return to the full file time range; X Start and X Stop fields show the file's actual start and stop times |
| 8 | Uncheck a series in the legend | That series disappears from the chart |
| 9 | Recheck it | Series reappears |
| 10 | Hover mouse over a data point on the chart | Tooltip shows series name, time (DDD:HH:MM:SS), and amplitude in dB |
| 11 | Move mouse away from data points | Tooltip disappears |
| 12 | Press Ctrl+E with the plot area focused | Legend toggles between Expand All and Collapse All |

### MT-09c: PDF Export

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | With data loaded, click Export PDF button | File save dialog with .pdf filter appears |
| 2 | Save to a test path | Log shows success with clickable PDF file link |
| 3 | Open the PDF | Plot renders correctly at high quality; title, axes, and legend visible |
| 4 | Set a custom title and Y range, then export again | PDF reflects the custom title and axis range |

### MT-09d: Copy Data to Clipboard

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | With data loaded, zoom to a sub-range using X Start/Stop | Chart shows only the zoomed time window |
| 2 | Click Copy Data | Log shows "Copied N rows to clipboard." |
| 3 | Paste into Excel or a text editor | Tab-separated table with Time column and one column per visible series; rows match the zoomed range |
| 4 | Uncheck some series in the legend, then click Copy Data again | Unchecked series are absent from the pasted table |

**Pass criteria**: Plot renders correctly; all interactive controls work; PDF output is correct; clipboard export matches visible data.

---

## MT-10: Batch Processing

**Purpose**: Verify sequential batch file processing.

### MT-10a: Basic Batch Run

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open multiple .ch10 files (File > Open, select all test files) | File list tree shows all files; each with Time/PCM channel dropdowns |
| 2 | Verify per-file channel dropdowns | Each file shows its own available channels |
| 3 | Set the PCM channel to the last channel for each NRZ-L file | Dropdowns update per file |
| 4 | Click Process | Batch output directory dialog appears |
| 5 | Select output directory | Processing begins; progress bar shows overall + per-file progress |
| 6 | Observe file list during processing | Status column shows colors (green = done, yellow = skip, red = error) |
| 7 | After completion | Log shows summary: success/skip/error counts |
| 8 | Verify output directory | One CSV per file, named `AGC_<inputname>.csv` |
| 9 | Observe "View AGC Plot" dialog | Dialog appears with a dropdown listing each successfully processed file |
| 10 | Select a file from the dropdown and click OK | Plot panel loads the selected CSV and displays AGC traces |
| 11 | Dismiss the dialog with Cancel | Plot is not loaded; application returns to normal state |

### MT-10b: Cancel Batch

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Start batch processing | Cancel button becomes visible in toolbar |
| 2 | Click Cancel | Processing stops after current file finishes; log shows cancellation summary |

### MT-10c: Batch with Invalid File

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Include a file whose selected PCM channel doesn't match the frame structure | That file is skipped with a yellow warning in the file list |
| 2 | Remaining files continue processing | Batch continues to completion |

**Pass criteria**: All valid files produce output CSVs; batch naming correct; cancel works; errors do not halt remaining files; "View AGC Plot" dialog appears after successful batch and loads selected CSV into the plot panel.

---

## MT-11: Keyboard Shortcuts

**Purpose**: Verify all documented keyboard shortcuts function.

| Shortcut | Expected Action |
|----------|-----------------|
| Ctrl+O | Opens file dialog (single file mode) |
| Ctrl+R | Starts processing (if inputs are valid) |

**Pass criteria**: Both shortcuts trigger their actions from any focus state.

---

## MT-12: Log Window Behavior

**Purpose**: Verify log window formatting, persistence, and auto-scroll.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Open a file | Log appends channel info in plain text |
| 2 | Process a file successfully | Log appends a green success message |
| 3 | Trigger an error (e.g., no receivers selected) | Log appends a red error message |
| 4 | Trigger a warning (e.g., parameter count mismatch in INI) | Log appends a dark-yellow (#DAA520) warning |
| 5 | Process multiple files | Log accumulates entries; earlier entries not cleared |
| 6 | Add many log entries | Log auto-scrolls to the most recent entry |
| 7 | Manually scroll up | Scrollbar works; earlier entries readable |

**Pass criteria**: Colors correct; log never clears; auto-scroll works.

---

## MT-13: Installer (US7)

**Purpose**: Verify the installer and portable distribution.

### MT-13a: EXE Installer (Admin)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Run `agcCh10toCSV_setup.exe` as Administrator | "What's New" page appears showing release notes |
| 2 | Click Next through wizard; select default install path | Progress bar advances during installation |
| 3 | After install, launch from Start Menu shortcut | Application starts; settings folder exists at `{install}/settings/` |
| 4 | Double-click a `.ch10` file in Explorer | Application opens and loads the file (file association works) |
| 5 | Run installer again (upgrade) | Existing `default.ini` not overwritten; new fields added as `new_default.ini` (if schema changed) |

### MT-13b: Non-Admin Install

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Run installer without admin rights | Installer offers user-directory install option |
| 2 | Complete install to user directory | Application runs; settings stored in user-writable path |

### MT-13c: Portable ZIP

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | Extract portable ZIP to a USB drive or arbitrary folder | Folder contains `agcCh10toCSV.exe`, `settings/default.ini`, `portable` marker, `LICENSE.txt`, `README.txt` |
| 2 | Launch `agcCh10toCSV.exe` from the extracted folder | App runs; QSettings stored locally (not in `HKCU` registry) |
| 3 | Move the folder to a different path and relaunch | App still runs correctly; settings preserved |

**Pass criteria**: Installer deploys all files; file association works; portable mode uses local settings.

---
