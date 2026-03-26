# agcCH10toCSV.md - AI Assistant Guide

This file provides context and guidelines for AI assistants working on the agcCh10toCSV (Chapter 10 to CSV AGC Converter) project.

## Version Information

- **Qt Version**: 6.10.2 (minimum: Qt 6.0.0)
- **MinGW Version**: 13.1.0 (minimum: GCC/MinGW 7.0)
- **C++ Standard**: C++17 (required — `inline constexpr` used throughout constants.h)
- **Project Version**: 3.1.2 — defined in `AppVersion` struct in `include/constants.h`

- **Target Users:** Telemetry engineers and data analysts who need to convert RCC IRIG 106 chapter 10 formated files that include automatic gain control (AGC) signals information from telmetery receivers to comma separated values so AGC signals can be plotted and analyzed in capplications like Microsoft Excel or Matlab. 

## Project Overview

**agcCh10toCSV** is a Qt 6 desktop application that converts AGC sample data in IRIG 106 Chapter 10 telemetry recording files (.ch10) to CSV formatted files. The application is built using Qt Widgets with Windows 11 styled dark and light themes.

## User Stories (v1.0 Requirements)

### US1: Export AGC signal data
**As an** As a telemetry engineer or data analyst
**I want to** to export AGC signal samples that include samples embedded in IRIG 106 Chapter 4 formated Pulse Code Modulation (PCM) streams in .ch10 files to a csv formatted file.  
**So that** so that I can import the samples into 3rd party applications like Excel/Matlab to process and visual AGC signal samples. I also want to see the progress of the conversion process and define the filename and location of the output file.

**Acceptance Criteria:**
- ✅ Data is extracted from a user provided .ch10 file and exported to a csv file
- ✅ Exported CSV files include at least the following: Sample Time, Raw Sample Value, etc.
- ✅ The first row of each column has a title that describes the data in the column
- ✅ Progress bar updates as the application is processing and converting the AGC samples.
- ✅ A file dialog is used to define the output filename and location.


### US2: Select the AGC sample and time channels
**As an** As a telemetry engineer or data analyst
**I want to** to be able select the .ch10 file channels that include the AGC samples and time.
**So that** so that that I can export data from .ch10 files with different PCM and time channels.

**Acceptance Criteria:**
- ✅ AGC data sample channel is selectable
- ✅ Time channel is selectable
- ✅ Only PCM channels are selectable for AGC data samples
- ✅ Only Time channels are selectable for time

### US3: Define information about the PCM stream of the data in the .ch10 file
**As an** As a telemetry engineer or data analyst
**I want to** define the PCM frame synchronization (sync) information and PCM code format of the AGC sample data the application uses when processing and exporting sample data.
**So that** so that I can export samples from .ch10 files with different frame sync information and PCM code formats.

**Acceptance Criteria:**
- ✅ User defined Frame sync pattern and length is used to determine the start of new frames
- ✅ Application ensures frame sync pattern only contains hexadecimal values
- ✅ Application ensures frame sync pattern is no larger than the user defined frame length
- ✅ PCM code format can be defined; NRZ-L or RNRZ-L
- ✅ Samples from the complete start and stop time of the ch10 file is exported
- ✅ Only samples from the time window defined by the user are exported
- ✅ Default frame sync information and PCM format is extracted from a configuraion file

### US4: Select specific receiver/channel samples and time window to export
**As an** As a telemetry engineer or data analyst
**I want to** to be able to identify which receiver's and channel's sample data and the sample rate (e.g. 1 Hz, 10 Hz, etc.) that is included in the export csv file as well as the time window of the data to be exported.
**So that** so that I can process and visualize from multiples receivers and receiver channels over a period of time that is important to me. I also want to export all reciver channels and the complete time duration of the .ch10 file.

**Acceptance Criteria:**
- ✅ All receiver and receiver channel samples are exported
- ✅ Only select reciver and receiver channel samples are exported
- ✅ Samples from the complete start and stop time of the ch10 file are exported
- ✅ Only samples from a user defined time window are exported
- ✅ Samples are averaged over a sample rate defined by the user

### US5: Convert AGC samples from Volts (V) to decibels (dB)
**As an** As a telemetry engineer or data analyst 
**I want to** I want to convert the raw integer value representation of the AGC sample amplitude in V to its dB equivalent
**So that** I can process and visualize the data in its original unit of measure.

**Acceptance Criteria:**
- ✅ Samples are converted from integer V to decimal dB values
- ✅ Uses my defined polarity (positive/negative) and slope (dB/V) to properly convert from integer V to decimal dB
- ✅ Default polarity and slope values are extracted from a configuraion file

### US6: View AGC samples
**As an** As a telemetry engineer or data analyst 
**I want to** I want view the processed agc samples vs. time in a plot window 
**So that** I can quickly analyze data and export plots to a PDF report equivalent

**Acceptance Criteria:**
- [x] Dockable plot window
- [x] User defined plot title
- [x] Y and X axis labels
- [x] Auto set Y axis to min/max values
- [x] Auto set X axis to the max time span in the processed csv file
- [x] Controls to set a time window; auto zoom and move the x-axis so only this time window is visible
- [x] Contol to overide Y axis min/max
- [x] Mouse wheel zooms the y-axis
- [x] Mouse click and hold pans the axis
- [x] Select and Unselect which reciever channel AGC values are visible/exported to PDF
- [x] Auto set plot colors by default; channels from the same receiver should be a shades of the same color

### US7: Error Checking
**As an** As a telemetry engineer or data analyst 
**I want to** ensure the values I enter into the application are valid
**So that** I can avoid errors and ensure the data I export is accurate

**Acceptance Criteria:**
- ✅ Ensure all input fields are validated
- ✅ Error messages are displayed to the user when invalid values are entered

### US8: Application Installer
**As a** developer
**I want to** create an application installer
**So that** I can quickly deploy the software/updates to users with all the necessary folders and settings files

**Acceptance Criteria:**
- ✅ Signed application
- ✅ Install to "Program Files" and to a user-selected directory for users without admin privileges
- ✅ Installer shows install progress
- ✅ Installer should not overwrite INI files; if new fields are in the INI file, alert the user that a new INI file was saved as "new_x.INI" — try to use as many parameters from the old default INI file as possible in the new INI file

## Version History

### v1.0 — Initial Release
- All v1.0 user stories complete (US1–US5 above)
- Automated unit tests (Qt Test framework)

### v2.0 — Code Quality & Usability
- ✅ Drag-and-drop .ch10 file loading
- ✅ Dark and light theme toggle with Windows 11 / WinUI 3 styling
- ✅ QComboBox dropdown chevron arrows respecting theme
- ✅ QToolButton hover/pressed states
- ✅ Persistent last-opened directory via QSettings
- ✅ All static functions moved into classes
- ✅ Magic numbers extracted to named constants
- ✅ Performance: pre-allocated buffers, throttled progress, batched CSV writes
- ✅ Full Doxygen @file/@brief documentation on all source files
- ✅ Version defined in AppVersion struct (no more #defines)

### v2.1.0 — Logging, Validation & UX
- ✅ Per-file-type directory persistence (Ch10, CSV, INI remember directories independently)
- ✅ Inline log window with colored entries: errors in red, warnings in dark yellow (#DAA520)
- ✅ Replaced QMessageBox error/warning dialogs with inline log entries (kept About dialog)
- ✅ Persistent log window — no longer cleared between processing runs; auto-scrolls on new entries
- ✅ Startup logging — logs default.ini settings (FrameSync, Polarity, Slope, Scale, Receivers, Channels, Frame setup count)
- ✅ Ch10 file opening logging — logs channel info, time range, and current frame settings
- ✅ INI load/save logging — logs all settings values when loading or saving INI files
- ✅ INI parameter section count validation — warns when parameter sections don't match receiver x channel count
- ✅ Dynamic frame length — frame size computed from receiver configuration instead of hardcoded
- ✅ Frame structure separated from channel selection — frame setup from default.ini defines PCM frame; receiver grid controls output
- ✅ Negative polarity calibration fix
- ✅ Excel-compatible time format in CSV output

### v2.1.5 — MVVM & Log Enhancements
- ✅ SettingsDialog MVVM data flow — `setData()`/`getData()` with `SettingsData` struct replaces 18 individual getter/setter calls
- ✅ Removed redundant `applySettings()` 6-parameter method; `applySettingsData()` is now the single entry point
- ✅ Removed dock widget title bar for cleaner controls panel layout
- ✅ Pre-scan re-runs automatically when user selects a different PCM channel
- ✅ Color-coded log messages: green for pre-scan success and processing complete, yellow for warnings, red for errors
- ✅ SettingsDialog unit tests for `setData()`/`getData()` roundtrip and non-edited field preservation

### v2.2.0 — Usability Improvements
- ✅ Quick receiver selection shortcuts (Select All / Select None buttons)
- ✅ Status bar with file metadata summary (filename, size, channel counts, time range)
- ✅ Read-only settings summary panel on main window (Sync, Polarity, Slope, Scale, Receivers)
- ✅ Pre-process summary in log (input file, channels, time range, sample rate, receivers, output path)
- ✅ Recent files menu (File > Recent Files) with persistence across sessions
- ✅ Clickable output file path and "Open Folder" link in log window (QTextBrowser)
- ✅ Removed success QMessageBox dialog — output path and folder link in log replace it
- ✅ Drag-and-drop Ch10 files (completed in v2.0)

### v2.3.0 — Easy Deployment (US7)
- ✅ Inno Setup EXE installer with admin/non-admin install mode (`PrivilegesRequiredOverridesAllowed=dialog`)
- ✅ Installer shows install progress (built into Inno Setup)
- ✅ INI file upgrade logic — post-install Pascal script merges old `default.ini` values into new, backs up as `default_old.ini`
- ✅ Portable ZIP distribution — flat layout with `portable` marker file for truly portable mode (QSettings stored locally)
- ✅ App root auto-detection — supports dev (`release/`), installed (`bin/`), and portable (flat) directory layouts
- ✅ Code signing via Windows Certificate Store (`SIGN_CERT_SHA1`); Inno Setup signs installer and uninstaller
- ✅ `.ch10` file association — optional installer task registers double-click-to-open
- ✅ Installer "What's New" page — shows release notes before install (`InfoBeforeFile`)
- ✅ Fixed resource file — version 2.3.0, `VFT_APP` file type, proper metadata and relative icon path
- ✅ Build automation script (`deploy/build_release.cmd`) — builds, stages, signs, and packages both artifacts
- ✅ Portable ZIP includes LICENSE.txt and README.txt
- ✅ Default Ch10 file dialog opens to application directory on first use
- ✅ Settings dialog button order — Cancel before OK (left to right)

### v2.4.0 — Batch Ch10 Processing
- ✅ Multi-file selection via file dialog (`QFileDialog::getOpenFileNames`) and drag-and-drop
- ✅ Sequential queue-based batch processing — one file at a time on worker thread
- ✅ Per-file pre-scan for encoding detection (NRZ-L vs RNRZ-L)
- ✅ Per-file channel validation — skips files missing the selected channel with a warning
- ✅ Per-file Time and PCM channel selection via embedded `QComboBox` widgets in the file list tree
- ✅ Unified `QTreeWidget` file list for both single-file and batch modes (replaced standalone channel dropdowns)
- ✅ Color-coded file status in tree: green (Valid/Done), yellow (Skip), red (Error), with encoding column
- ✅ Batch output naming: `AGC_<input_basename>.csv` in a user-selected output directory
- ✅ Overall progress bar: `((fileIndex * 100) + perFilePercent) / totalFiles`
- ✅ Batch cancellation — dedicated cancel toolbar button visible during processing
- ✅ Continue-on-error with summary report (success/skip/error counts)
- ✅ `BatchFileInfo` value struct for per-file metadata tracking
- ✅ Batch ViewModel unit tests (`TestMainViewModelBatch`)

### v3.0.0 — AGC Signal Plot Window (US6)
- ✅ QCustomPlot 2.1.1 charting library integrated (GPL, two-file library in `lib/qcustomplot/`)
- ✅ PlotViewModel: CSV parsing to in-memory series data, axis range management, color assignment
- ✅ PlotWidget: QCustomPlot chart with interactive controls (title, Y/X range spinboxes, reset button)
- ✅ Dockable plot window (right QDockWidget) — auto-shown after single-file processing
- ✅ User-defined plot title via editable QLineEdit
- ✅ Labeled X (elapsed seconds) and Y (dB) axes
- ✅ Auto-scale Y axis to min/max values on data load; manual override via spinboxes
- ✅ Auto-scale X axis to full time span; time window controls with start/stop spinboxes
- ✅ X axis clamped at 0 — user cannot pan below zero elapsed time
- ✅ Mouse wheel zoom (Y axis) and click-drag pan (both axes)
- ✅ Per-receiver-channel visibility toggle via legend checkboxes
- ✅ Auto-assigned plot colors: 10-hue palette, channels from same receiver share hue with varied saturation
- ✅ Log window restructured as bottom QDockWidget (was central widget)
- ✅ Log auto-hides when plot opens; auto-restores when plot closes
- ✅ View menu (Show Plot / Show Log) between File and Help menus
- ✅ Chart theme syncs with dark/light application theme toggle
- ✅ QGroupBox → QWidget migration for ReceiverGridWidget and TimeExtractionWidget with QFrame separators
- ✅ Translucent receiver tree backgrounds in both left pane and plot legend
- ✅ Sample rate stays enabled in batch mode (time fields disabled, sample rate active)
- ✅ Fixed file list height — consistent size before and after file load
- ✅ Plot controls disabled until data loads (title, spinboxes, reset, mouse interactions)
- ✅ Performance: `rpQueuedReplot` coalesces redundant replots; graph visibility toggle without full rebuild; per-series Y min/max caching; pre-allocated CSV parse vectors
- ✅ PlotViewModel unit tests (13 test cases)
- ✅ PlotConstants test coverage in TestConstants

### v3.1.0 — Plot Enhancements
- ✅ Export PDF button in plot window — saves current plot to PDF file
- ✅ File save dialog with .pdf filter and automatic extension appending
- ✅ Success/error logging with clickable file link in log window
- ✅ Export button disabled when no data is loaded (prevents invalid state)
- ✅ Uses QCustomPlot's built-in `savePdf()` method for high-quality output
- ✅ 4-pixel horizontal spacing between axis control buttons for improved layout
- ✅ X axis displays actual file time (DDD:HH:MM:SS) instead of elapsed seconds
- ✅ Custom `TimeHackTicker` axis ticker with 10 major tick marks
- ✅ `PlotViewModel::formatTime()` converts elapsed seconds to absolute time using base DOY/time
- ✅ Batch mode plot support (plot per file or overlay)

### v3.1.1 — Code Quality & Test Coverage
- ✅ Clang-tidy efficiency improvements: removed unused variable, `NULL` → `nullptr` modernization (6 instances), `int` → `qsizetype` narrowing fixes (7 loops)
- ✅ Simplified iterator loop to `QMap::keys()` in PlotWidget legend builder
- ✅ TestFrameProcessor unit tests (17 test cases) — covers constructor, abort flag, `hasSyncPattern()`, `derandomizeBitstream()`, `writeTimeSample()`, `preScan()`, and `process()` with real Ch10 test files
- ✅ TestTimeExtractionWidget unit tests (8 test cases) — covers defaults, signal emission, time population/clearing, enable/disable controls, sample rate options
- ✅ TestReceiverGridWidget unit tests (7 test cases) — covers construction, rebuild, mass check/uncheck, Select All/None signal emission, edge cases (0 and 1 receivers)

### v3.1.2 — Code Quality & UX Polish
- ✅ Fixed dangling `m_current_processor` pointer after worker thread cleanup
- ✅ Initialized all PlotWidget member pointers to `nullptr`
- ✅ Added `QDoubleSpinBox`/`QSpinBox` dark and light theme styling
- ✅ Added keyboard shortcuts: Ctrl+O (Open), Ctrl+R (Process)
- ✅ Added busy cursor during file processing
- ✅ Added tooltips to all plot controls (spinboxes, buttons, title)
- ✅ Fixed README sample rate typo (20 Hz → 100 Hz)
- ✅ Updated portable README version to current release

## Future Version Functions

### v3.2.0 — UX Improvements
- [ ] Import previously exported CSV files to view in the plot
- [ ] Auto dark/light theme based on Windows system theme (`QStyleHints::colorScheme()`, Qt 6.5+)

## Tech Stack

- **Language**: C++17
- **Framework**: Qt 6.10.2 (Widgets module)
- **Build System**: qmake
- **Compiler**: MinGW 13.1.0 (64-bit) on Windows
- **Platform**: Windows (primary target)
- **External Libraries**: irig106utils (embedded C library), QCustomPlot 2.1.1 (embedded charting library, `lib/qcustomplot/`)

## ⚠️ CRITICAL: Protected Files - DO NOT MODIFY

The following files are third-party library code and **MUST NOT be modified** under any circumstances:

### Protected Source Files
- `lib/irig106/src/irig106*.c` - All IRIG 106 C source files
- `lib/irig106/src/i106_*.c` - All i106 prefixed C source files
- `lib/qcustomplot/qcustomplot.cpp` - QCustomPlot charting library

### Protected Header Files
- `lib/irig106/include/irig106*.h` - All IRIG 106 header files
- `lib/irig106/include/i106_*.h` - All i106 prefixed header files
- `lib/irig106/include/config.h` - IRIG 106 configuration
- `lib/qcustomplot/qcustomplot.h` - QCustomPlot charting library header

**File Patterns to Exclude**: Any file containing `i106` or `irig106` in its name; any file in `lib/qcustomplot/`

**Reason**: These files are from external libraries ([irig106utils](https://github.com/atac/irig106utils), [QCustomPlot](https://www.qcustomplot.com/)) and are maintained separately. Modifications would:
- Break compatibility with the upstream library
- Make future updates difficult
- Potentially introduce bugs in tested code

**If changes are needed**: They should be made by wrapping/adapting the library in application code (e.g., `chapter10reader.cpp`, `frameprocessor.cpp`, `plotwidget.cpp`), NOT by modifying the library files directly.

## Architecture

### Core Components

The application follows the **MVVM (Model-View-ViewModel)** pattern:

1. **MainView** (`src/mainview.cpp`, `include/mainview.h`) — *View*
   - Thin GUI layer; creates and lays out all Qt widgets
   - Delegates receiver grid to ReceiverGridWidget, time controls to TimeExtractionWidget
   - Binds to MainViewModel Q_PROPERTYs and connects signals/slots
   - Contains no business logic; delegates all actions to the ViewModel
   - `logError()` / `logWarning()` / `logSuccess()` append colored HTML entries (red / #DAA520 / green) to the log window via `append()`
   - Errors and warnings are shown inline in the log; QMessageBox reserved for About dialog only
   - Log window uses `QTextBrowser` for clickable links; persistent (never cleared) with auto-scroll on new entries
   - Status bar displays file metadata summary (filename, size, channel counts, time range)
   - Read-only settings summary panel shows current frame sync, polarity, slope, scale, and receiver configuration
   - Recent Files submenu under File menu with persistence across sessions
   - Unified `QTreeWidget` file list for both single-file and batch modes with embedded per-file Time and PCM channel `QComboBox` selectors
   - Color-coded per-file status (Ready/Valid/Done/Skip/Error) and encoding column in batch mode
   - Multi-file selection via `QFileDialog::getOpenFileNames()` and multi-file drag-and-drop
   - Batch output directory prompt via `QFileDialog::getExistingDirectory()`
   - Dedicated cancel toolbar button (visible only during processing)
   - Log window in bottom QDockWidget; auto-hides when plot dock opens, restores when plot closes
   - Plot dock (right QDockWidget) with PlotWidget; auto-shown after single-file processing
   - View menu (Show Plot / Show Log) between File and Help menus

   a. **ReceiverGridWidget** (`src/receivergridwidget.cpp`, `include/receivergridwidget.h`) — *View*
      - Self-contained multi-column tree grid for receiver/channel selection
      - Manages expand/collapse, tri-state checkboxes, and synchronized scrollbars
      - Emits `receiverChecked()` when the user toggles a channel checkbox
      - Emits `selectAllRequested()` / `selectNoneRequested()` from dedicated buttons

   b. **TimeExtractionWidget** (`src/timeextractionwidget.cpp`, `include/timeextractionwidget.h`) — *View*
      - Widget with extract-all toggle, start/stop time inputs, and sample rate selector
      - `setSampleRateEnabled()` keeps sample rate active when other time controls are disabled (batch mode)
      - Emits `extractAllTimeChanged()` and `sampleRateIndexChanged()` signals

2. **SettingsDialog** (`src/settingsdialog.cpp`, `include/settingsdialog.h`) — *View*
   - Modal dialog for frame sync, polarity, scale, range, and receiver settings
   - Uses `setData()`/`getData()` with `SettingsData` struct for clean data transfer
   - Emits `loadRequested()` and `saveAsRequested()` for file I/O delegation

3. **MainViewModel** (`src/mainviewmodel.cpp`, `include/mainviewmodel.h`) — *ViewModel*
   - Owns all application state, validation, and processing orchestration
   - Exposes Q_PROPERTYs for the View to bind to
   - `validateProcessingInputs()`, `prepareFrameSetupParameters()`, `launchWorkerThread()` orchestrate the processing pipeline
   - `validateTimeFields()` shared by start/stop time validation; `generateOutputFilename()` shared by input-success and processing-finished flows
   - Creates a fresh `FrameProcessor` per processing run on a worker thread
   - `logStartupInfo()` emits default.ini settings at application startup (called after signal connections are established)
   - `openFile()` logs channel info, time range, and current frame settings when a Ch10 file is loaded
   - `runPreScan()` detects PCM encoding and verifies frame sync; runs on file open and on PCM channel change
   - `fileMetadataSummary()` returns formatted string for the status bar
   - `recentFiles()`, `addRecentFile()`, `clearRecentFiles()` manage recent file list with QSettings persistence
   - Emits pre-process summary log messages before launching worker thread
   - Batch processing: `openFiles()` loads multiple files, per-file channel discovery and validation
   - `setBatchFilePcmChannel()` / `setBatchFileTimeChannel()` for per-file channel selection
   - `startBatchProcessing(output_dir, sample_rate_index)` / `processNextBatchFile()` drive sequential batch execution with async continuation via `onProcessingFinished()`

4. **PlotViewModel** (`src/plotviewmodel.cpp`, `include/plotviewmodel.h`) — *ViewModel*
   - Parses CSV output files into in-memory `PlotSeriesData` vectors (name, receiver index, x/y values, cached Y min/max, color)
   - Pre-allocates data vectors from estimated file size for efficient CSV parsing
   - Converts DOY + HMS timestamps to elapsed seconds from first sample
   - Assigns colors from a 10-hue palette; channels within same receiver get varied saturation/value
   - Manages axis ranges (auto Y with margin, manual Y override, X time window)
   - Per-series visibility toggle; signals `dataChanged()`, `axisRangeChanged()`, `seriesVisibilityChanged()`
   - `computeYRange()` uses per-series cached min/max (O(series) not O(data points))

5. **PlotWidget** (`src/plotwidget.cpp`, `include/plotwidget.h`) — *View*
   - Self-contained QCustomPlot chart widget with toolbar controls and legend panel
   - Top toolbar: title QLineEdit
   - Axis controls grid: X start/stop and Y min/max spinboxes in aligned columns, reset button
   - Legend: scrollable colored tree checkboxes for per-series visibility
   - Supports mouse wheel zoom (Y axis) and click-drag pan (both axes)
   - `onSeriesVisibilityToggled()` toggles individual graph visibility without full rebuild
   - All replots use `rpQueuedReplot` to coalesce redundant repaint requests
   - All plot controls disabled until data loads; enabled in `rebuildChart()`
   - `applyTheme(bool dark)` syncs chart colors with app dark/light theme
   - Placed inside a right QDockWidget by MainView

6. **Chapter10Reader** (`src/chapter10reader.cpp`, `include/chapter10reader.h`) — *Model*
   - Reads IRIG 106 Chapter 10 file metadata and manages channel selection
   - Scans TMATS records to catalog time and PCM channels
   - Provides channel lists, time accessors, and channel ID resolution
   - Wraps irig106utils C library for file I/O

7. **FrameProcessor** (`src/frameprocessor.cpp`, `include/frameprocessor.h`) — *Model*
   - Self-contained PCM frame extraction and CSV output processor
   - Created fresh per processing run, moved to a worker thread, auto-deleted via `deleteLater`
   - Owns its own irig106 file handle, buffers, and TMATS metadata
   - `process()` method takes channel IDs (not indices) and emits progress/completion signals
   - Private helper methods: `freeChanInfoTable()`, `assembleAttributesFromTMATS()`, `derandomizeBitstream()`, `hasSyncPattern()`

8. **SettingsManager** (`src/settingsmanager.cpp`, `include/settingsmanager.h`) — *Model*
   - Handles saving/loading user preferences using QSettings
   - Persists UI state between sessions via `MainViewModel*`
   - Validates all INI values on load (FrameSync hex, Polarity, Slope, Scale, receiver count/channels)
   - Validates parameter section count against receiver x channel configuration
   - Emits `logMessage()` for load/save status, warnings, and errors routed to the log window

9. **FrameSetup** (`src/framesetup.cpp`, `include/framesetup.h`) — *Model*
   - Manages frame configuration parameters (word map, calibration)
   - Handles frame setup file loading and saving

10. **IRIG 106 Library** (`lib/irig106/src/irig106*.c`, `lib/irig106/include/i106*.h`)
   - Third-party C library for Chapter 10 file format
   - Handles low-level file parsing and data structures

### Constants and Data Structures

- **`AppVersion`** struct (in `include/constants.h`) — Version information with `kMajor`, `kMinor`, `kPatch` and `toString()`
- **`PCMConstants`** namespace (in `include/constants.h`) — Named constants for PCM frame parameters (word count, frame length, sync pattern length, time rounding, channel type identifiers, max raw sample value, default buffer size, progress report interval)
- **`UIConstants`** namespace (in `include/constants.h`) — Named constants for UI configuration (QSettings keys, theme identifiers, receiver grid layout, time conversion, receiver count, default slope/scale, button text, time validation limits, sample rates, output filename format, deployment/portable mode constants)
- **`SettingsData`** struct (in `include/settingsdata.h`) — Value type used to transfer UI state between MainViewModel and SettingsManager without `friend class` coupling
- **`BatchFileInfo`** struct (in `include/batchfileinfo.h`) — Per-file metadata for batch processing (filepath, channel strings/IDs, resolved channel indices, validation state, encoding, processing result)
- **`PlotConstants`** namespace (in `include/constants.h`) — Named constants for plot dock dimensions, axis margin factor, default title, axis labels, zoom factor, and receiver color palette (10 hues)
- **`PlotSeriesData`** struct (in `include/plotviewmodel.h`) — Per-series data for plotting (name, receiver/channel indices, x/y value vectors, visibility, color, cached Y min/max)
- **`SuChanInfo`** typedef (in `include/frameprocessor.h`) — Per-channel bookkeeping struct for the irig106 C helper layer

### Data Flow

```
User Input → MainView → MainViewModel → Chapter10Reader (metadata)
                              ↓                ↓
                        SettingsManager    FrameSetup
                              ↓
                        FrameProcessor → IRIG106 Library
                              ↓
                          CSV Output
                              ↓
                        PlotViewModel → PlotWidget (QCustomPlot)
```

## Qt-Specific Considerations

### Qt Version Compatibility

- **Target**: Qt 6.10.2
- **Important**: Qt 6 made significant changes to container classes
  - `QStringList` methods differ from Qt 5
  - Prefer range-based for loops when iterating over Qt containers

### Container Iteration Pattern

**Preferred** (range-based for loop):
```cpp
QStringList items = getItems();
for (const QString& item : items)
    doSomething(item);
```

**Avoid** (index-based with .length()):
```cpp
// This may not compile in Qt 6!
for (int i = 0; i < items.length(); i++)
    doSomething(items[i]);
```

### Required Includes

When using Qt classes, ensure proper headers are included:
- `QStringList` requires `#include <QStringList>`
- `QString` requires `#include <QString>`
- Composite widgets require their specific headers

## Coding Conventions

### File Organization
- **Headers**: `include/` directory
- **Implementation**: `src/` directory
- **Resources**: `resources/` directory
- **UI files**: If using Qt Designer (currently hand-coded)

### Naming Conventions
- **Classes**: PascalCase (e.g., `MainView`, `MainViewModel`, `Chapter10Reader`, `FrameProcessor`)
- **Constants**: kPascalCase in namespaces (e.g., `PCMConstants::kWordsInMinorFrame`, `UIConstants::kDefaultScaleIndex`)
- **Member variables**: m_ prefix with snake_case (e.g., `m_frame_setup`, `m_reader`); widget members drop type suffixes when the declared type is clear (e.g., `m_input_file` not `m_input_file_lineedit`); buttons use `_btn` suffix (e.g., `m_process_btn`); settings members use `m_settings_` prefix (e.g., `m_settings_frame_sync`)
- **Methods**: camelCase (e.g., `process()`, `getTimeChannelComboBoxList()`)
- **Slots**: camelCase with descriptive names (e.g., `inputFileButtonPressed()`)
- **Struct fields**: `ParameterInfo` and `ProcessingParams` use snake_case; `SettingsData` uses camelCase (Qt property style)

### Memory Management
- UI widgets created with `new` should specify parent widget for automatic cleanup
- Manual `delete` in destructor for widgets without parents
- Follow Qt's parent-child ownership model
- Use `const QString&` for all string parameters passed by reference

### Include Ordering (Google C++ Style)
Order includes in each `.cpp` / `.h` file as follows, with a blank line between groups:
1. Related header (e.g., `#include "mainview.h"` in `mainview.cpp`)
2. C system headers (e.g., `<time.h>`)
3. C++ standard library headers (e.g., `<fstream>`, `<string>`)
4. Third-party / Qt headers (e.g., `<QDebug>`, `<QMessageBox>`) — alphabetized
5. irig106 library headers — keep in dependency order, not alphabetized
6. Project headers (e.g., `"channeldata.h"`, `"constants.h"`) — alphabetized

### Signals and Slots
- Use Qt's signals/slots mechanism for event handling
- Connect signals in `setUpConnections()` method
- Uses new-style connect syntax (`&ClassName::signalName`)

## Build System

### qmake Project File (agcCh10toCSV.pro)
- Defines source files, headers, resources
- Configures Qt modules (core, gui, widgets)
- Sets C++17 standard
- Includes platform-specific libraries (ws2_32 for Windows sockets)

### Build Targets
- **Debug**: `mingw32-make -f Makefile.Debug` inside `build/` → `build/debug/agcCh10toCSV.exe`
- **Release**: `mingw32-make -f Makefile.Release` inside `build/` → `build/release/agcCh10toCSV.exe`

### VS Code Integration
Tasks are defined in `.vscode/tasks.json`:
- "qmake: Configure" - Runs qmake to generate Makefiles
- "Build (Debug)" - Compiles debug build
- "Build (Release)" - Compiles release build
- "Clean" - Cleans build artifacts
- "Rebuild" - Clean + Build

### Deployment & Packaging
- **Build automation**: `deploy/build_release.cmd` — builds release, runs `windeployqt`, stages installer and portable layouts, signs exe, creates ZIP, compiles Inno Setup installer
- **Inno Setup installer**: `deploy/agcCh10toCSV.iss` — EXE installer with admin/non-admin support, INI merge logic, `.ch10` file association, "What's New" page, Start Menu/desktop shortcuts
- **Portable ZIP**: Flat layout with `portable` marker file; QSettings redirected to app directory via `QSettings::setPath()` in `main.cpp`; includes LICENSE.txt and README.txt
- **Release notes**: `deploy/RELEASENOTES.txt` — shown as "What's New" page in installer (`InfoBeforeFile`)
- **Portable README**: `deploy/README_portable.txt` — copied as README.txt into portable ZIP
- **App root auto-detection** (`mainviewmodel.cpp`): Checks if `settings/` exists next to the exe (portable) or one level up (installed/dev/build)
- **Installed layout**: `{install}/bin/agcCh10toCSV.exe` + `{install}/settings/default.ini`
- **Portable layout**: `agcCh10toCSV.exe` + `settings/default.ini` + `portable` marker + `LICENSE.txt` + `README.txt` in same directory
- **Code signing**: Optional via `SIGN_CERT_SHA1` environment variable (SHA-1 thumbprint of certificate in Windows Certificate Store)

## Important Implementation Notes

### Chapter 10 File Handling
- Uses irig106utils library (C code, not C++)
- Be careful with C/C++ interop (no exceptions in C code)
- File handles managed through `m_file_handle`
- Buffer management for reading packets

### Time Handling
- Uses IRIG time format and standard time structures
- Time conversions between different formats (DOY/HMS ↔ uint64)
- UTC timezone enforced in Chapter10Reader and FrameProcessor constructors

### AGC Processing
- Central processing function: `FrameProcessor::process()`
- Takes parameters: input file, frame setup, output file, channel IDs, sync, sync length, time range, sample rate
- Returns bool indicating success/failure
- Created fresh per run by `MainViewModel::launchWorkerThread()`, moved to a background `QThread`, auto-deleted via `deleteLater` when the thread finishes
- Emits `progressUpdated(int)`, `processingFinished(bool)`, `logMessage(QString)`, and `errorOccurred(QString)` signals

## Common Development Tasks

### Adding a New UI Widget
1. Declare pointer in `mainview.h` private section
2. Create widget in appropriate `setUpSection()` method
3. Add to layout
4. Connect signals if needed in `setUpConnections()`
5. Handle cleanup in destructor if no parent
6. If the widget drives business logic, expose the action via a MainViewModel slot or property

### Modifying Build Configuration
1. Edit `agcCh10toCSV.pro`
2. Re-run qmake: `qmake agcCh10toCSV.pro`
3. Rebuild project

### Adding a New Setting
1. Add field to `SettingsData` struct in `include/settingsdata.h`
2. Add to `MainViewModel::getSettingsData()` and `MainViewModel::applySettingsData()`
3. Add to save logic in `SettingsManager::saveFile()`
4. Add to load logic in `SettingsManager::loadFile()`
5. Update UI initialization in `MainViewModel::clearState()`

## Debugging

### Common Issues

1. **Build fails with PATH errors**
   - Ensure Qt bin and MinGW bin are in PATH
   - Check paths in `.vscode/tasks.json` match your Qt installation

2. **MOC errors**
   - Ensure Q_OBJECT macro is present in classes with signals/slots
   - Re-run qmake if header structure changed

### Build Warnings
- Build should produce 0 warnings. If new warnings appear, fix them before committing.

## Testing

Automated unit tests use the **Qt Test** framework. Test sources are in the `tests/` directory with a separate `tests/tests.pro` project file.

### Test Suites
- **TestChannelData** (`tst_channeldata`) — ChannelData model object tests
- **TestConstants** (`tst_constants`) — Verifies all PCMConstants, UIConstants, AppVersion, and recent files constants (including kMaxPacketBufferSize, kFrameSyncHexPattern)
- **TestMainViewModelHelpers** (`tst_mainviewmodel_helpers`) — ViewModel helper methods (channelPrefix, parameterName, generateOutputFilename)
- **TestMainViewModelState** (`tst_mainviewmodel_state`) — ViewModel property defaults, setters, signals, receiver grid, SettingsData roundtrip, frame setup loading, recent files, file metadata summary
- **TestFrameSetup** (`tst_framesetup`) — Frame parameter loading, word map, calibration
- **TestSettingsDialog** (`tst_settingsdialog`) — SettingsDialog widget defaults, setter/getter roundtrips, SettingsData roundtrip, signal emission
- **TestSettingsManager** (`tst_settingsmanager`) — INI load/save validation (invalid FrameSync, Slope, Scale, Polarity, receiver counts, parameter count mismatch, roundtrip, frame setup preservation)
- **TestMainViewModelBatch** (`tst_mainviewmodel_batch`) — Batch mode defaults, generateBatchOutputFilename format, batchStatusSummary, clearState/cancelProcessing batch reset, per-file channel setter bounds checking
- **TestPlotViewModel** (`tst_plotviewmodel`) — PlotViewModel default state, CSV loading, time conversion, series color assignment, Y auto/manual range, X time window, series visibility, clear data, plot title, invalid/empty file handling
- **TestFrameProcessor** (`tst_frameprocessor`) — FrameProcessor constructor, abort flag, private static helpers (hasSyncPattern, derandomizeBitstream, writeTimeSample), preScan with valid/invalid files and encodings, process with real Ch10 test data
- **TestTimeExtractionWidget** (`tst_timeextractionwidget`) — Widget defaults, extractAllTime toggle, sampleRate setter/getter, fillTimes/clearTimes, enable/disable controls, sample rate options
- **TestReceiverGridWidget** (`tst_receivergridwidget`) — Widget construction, rebuild with tree items, mass check/uncheck, Select All/Select None signal emission, zero and single receiver edge cases

### Running Tests
```bash
cd tests
qmake tests.pro -spec win32-g++
mingw32-make -f Makefile.Debug
./debug/agcCh10toCSV_tests.exe -o results.txt,txt
```

### Adding a New Test
1. Create `tst_newtest.h` with `Q_OBJECT` and private slots for each test case
2. Create `tst_newtest.cpp` with test implementations
3. Add both files to `tests/tests.pro` under `SOURCES +=` and `HEADERS +=`
4. Add `#include "tst_newtest.h"` and a `QTest::qExec()` block in `tests/main.cpp`

## Future Feature Candidates

The following features have been identified as potential improvements for future versions. None are committed to a release — they are tracked here for planning purposes.

### UX Improvements

- **Theme hot-reload without restart** — Currently toggling between dark and light theme requires an application restart. The fix would reload the QSS stylesheet at runtime (`qApp->setStyleSheet()`) and call `PlotWidget::applyTheme()` to update chart colors immediately, with no restart needed.

- **Plot tooltip on hover** — When the mouse hovers over a data point on the plot, show a tooltip with the exact time (DDD:HH:MM:SS) and amplitude (dB) value. This would help users perform data accuracy spot-checks without needing to open the CSV.

- **Plot data export to clipboard** — A "Copy to Clipboard" button in the plot toolbar that copies the currently visible data range as tab-separated values, allowing quick paste into Excel for comparison.

- **Keyboard shortcuts for Expand/Collapse All** — The Receivers section and plot legend have Expand All / Collapse All buttons but no keyboard shortcuts. Adding `Ctrl+E` / `Ctrl+Shift+E` (or similar) would benefit keyboard-centric workflows.

### Batch Processing Improvements

- **Per-file retry after batch failure** — Currently, if a file fails during batch processing it is marked ERROR and the batch moves on. There is no way to retry a single failed file — the user must re-queue the entire batch. A "Retry Failed" button shown after batch completion would save time on large batches.

- **Drag-and-drop reordering of batch file list** — When multiple files are loaded for batch processing, their order is set by the selection order. Drag-and-drop reordering in the file list tree would give users control over processing sequence.

### Architecture / Robustness

- **Background CSV parsing** — `PlotViewModel::loadCsvFile()` runs on the main thread. For very large CSV outputs (long recordings, many channels), parsing can visibly delay the UI after clicking "Show Plot". Moving parsing to a background QThread with a progress signal would keep the UI responsive.

- **Efficiency: cache batch status summary** — `MainViewModel::inputFilename()` calls `batchStatusSummary()` on every property query, rebuilding the string each time. Caching the summary string and invalidating it only when the batch file list changes would eliminate redundant string allocation.

## Additional Resources

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [IRIG 106 Chapter 10 Standard](https://www.irig106.org/)
- [irig106utils GitHub](https://github.com/atac/irig106utils)
- [Qt Coding Conventions](https://wiki.qt.io/Qt_Coding_Style)
