# CLAUDE.md - AI Assistant Guide

This file provides context and guidelines for AI assistants working on the agcCh10toCSV (Chapter 10 to CSV AGC Converter) project.

## Version Information

- **Qt Version**: 6.10.2
- **MinGW Version**: 13.1.0
- **C++ Standard**: C++17
- **Project Version**: See MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION in chapter10reader.h (currently 0.9.0)

- **Target Users:** Telemetry engineers and data analysts who need to convert RCC IRIG 106 chapter 10 formated files that include automatic gain control (AGC) signals information from telmetery receivers to comma separated values so AGC signals can be plotted and analyzed in capplications like Microsoft Excel or Matlab. 

## Project Overview

**agcCh10toCSV** is a Qt 6 desktop application that converts AGC sample data in IRIG 106 Chapter 10 telemetry recording files (.ch10) to CSV formatted files. The application is built using Qt Widgets with a custom dark theme UI.

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

## Future Version Functions

### v1.0 — Release Candidate
- All v1.0 user stories complete (US1–US5 above)
- Automated unit tests (Qt Test framework)
- Installer / deployment packaging

### v1.1 — Usability Improvements
- Drag-and-drop .ch10 file loading
- Recent files list (File menu)
- Cancel/abort in-progress processing
- Status bar with file metadata summary

### v1.2 — Extended Format Support
- Configurable CSV delimiter (comma, tab, semicolon)
- Additional PCM code formats beyond NRZ-L and RNRZ-L
- Export to additional output formats (e.g., JSON, HDF5)
- Batch processing of multiple .ch10 files

### v2.0 — Analysis and Visualization
- Built-in AGC signal plotting (time-series chart)
- Real-time preview of AGC data during processing
- Statistical summary in output (min, max, mean, std dev)
- Command-line interface (CLI) mode for scripting and automation

## Tech Stack

- **Language**: C++17
- **Framework**: Qt 6.10.2 (Widgets module)
- **Build System**: qmake
- **Compiler**: MinGW 13.1.0 (64-bit) on Windows
- **Platform**: Windows (primary target)
- **External Library**: irig106utils (embedded C library)

## ⚠️ CRITICAL: Protected Files - DO NOT MODIFY

The following files are third-party IRIG 106 library code and **MUST NOT be modified** under any circumstances:

### Protected Source Files
- `lib/irig106/src/irig106*.c` - All IRIG 106 C source files
- `lib/irig106/src/i106_*.c` - All i106 prefixed C source files

### Protected Header Files
- `lib/irig106/include/irig106*.h` - All IRIG 106 header files
- `lib/irig106/include/i106_*.h` - All i106 prefixed header files
- `lib/irig106/include/config.h` - IRIG 106 configuration

**File Pattern to Exclude**: Any file containing `i106` or `irig106` in its name

**Reason**: These files are from the external [irig106utils library](https://github.com/atac/irig106utils) and are maintained separately. Modifications would:
- Break compatibility with the upstream library
- Make future updates difficult
- Potentially introduce bugs in tested telemetry parsing code

**If changes are needed**: They should be made by wrapping/adapting the library in `chapter10reader.cpp` or `frameprocessor.cpp`, NOT by modifying the library files directly.

## Architecture

### Core Components

The application follows the **MVVM (Model-View-ViewModel)** pattern:

1. **MainView** (`src/mainview.cpp`, `include/mainview.h`) — *View*
   - Thin GUI layer; creates and lays out all Qt widgets
   - Binds to MainViewModel Q_PROPERTYs and connects signals/slots
   - Contains no business logic; delegates all actions to the ViewModel

2. **ConfigDialog** (`src/configdialog.cpp`, `include/configdialog.h`) — *View*
   - Modal dialog for frame sync, polarity, scale, range, and receiver configuration
   - Reads initial values from MainViewModel and writes back on accept

3. **MainViewModel** (`src/mainviewmodel.cpp`, `include/mainviewmodel.h`) — *ViewModel*
   - Owns all application state, validation, and processing orchestration
   - Exposes Q_PROPERTYs for the View to bind to
   - `validateProcessingInputs()`, `prepareFrameSetupParameters()`, `launchWorkerThread()` orchestrate the processing pipeline
   - `validateTimeFields()` shared by start/stop time validation; `generateOutputFilename()` shared by input-success and processing-finished flows
   - Creates a fresh `FrameProcessor` per processing run on a worker thread

4. **Chapter10Reader** (`src/chapter10reader.cpp`, `include/chapter10reader.h`) — *Model*
   - Reads IRIG 106 Chapter 10 file metadata and manages channel selection
   - Scans TMATS records to catalog time and PCM channels
   - Provides channel lists, time accessors, and channel ID resolution
   - Wraps irig106utils C library for file I/O

5. **FrameProcessor** (`src/frameprocessor.cpp`, `include/frameprocessor.h`) — *Model*
   - Self-contained PCM frame extraction and CSV output processor
   - Created fresh per processing run, moved to a worker thread, auto-deleted via `deleteLater`
   - Owns its own irig106 file handle, buffers, and TMATS metadata
   - `process()` method takes channel IDs (not indices) and emits progress/completion signals
   - Absorbs the former `chapter10reader_helper.cpp` functions (`FreeChanInfoTable`, `AssembleAttributesFromTMATS`)

6. **SettingsManager** (`src/settingsmanager.cpp`, `include/settingsmanager.h`) — *Model*
   - Handles saving/loading user preferences using QSettings
   - Persists UI state between sessions via `MainViewModel*`

7. **FrameSetup** (`src/framesetup.cpp`, `include/framesetup.h`) — *Model*
   - Manages frame configuration parameters (word map, calibration)
   - Handles frame setup file loading and saving

8. **IRIG 106 Library** (`lib/irig106/src/irig106*.c`, `lib/irig106/include/i106*.h`)
   - Third-party C library for Chapter 10 file format
   - Handles low-level file parsing and data structures

### Constants and Data Structures

- **`PCMConstants`** namespace (in `include/constants.h`) — Named constants for PCM frame parameters (word count, frame length, sync pattern length, time rounding, channel type identifiers)
- **`UIConstants`** namespace (in `include/constants.h`) — Named constants for UI configuration (receiver count, default slope/scale, button text, time validation limits, sample rates, output filename format)
- **`SettingsData`** struct (in `include/settingsdata.h`) — Value type used to transfer UI state between MainViewModel and SettingsManager without `friend class` coupling
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
- **Member variables**: m_ prefix with snake_case (e.g., `m_frame_setup`, `m_reader`); widget members drop type suffixes when the declared type is clear (e.g., `m_input_file` not `m_input_file_lineedit`); buttons use `_btn` suffix (e.g., `m_process_btn`); config members use `m_cfg_` prefix (e.g., `m_cfg_frame_sync`)
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
- **Debug**: `mingw32-make -f Makefile.Debug` → `debug/agcCh10toCSV.exe`
- **Release**: `mingw32-make -f Makefile.Release` → `release/agcCh10toCSV.exe`

### VS Code Integration
Tasks are defined in `.vscode/tasks.json`:
- "qmake: Configure" - Runs qmake to generate Makefiles
- "Build (Debug)" - Compiles debug build
- "Build (Release)" - Compiles release build
- "Clean" - Cleans build artifacts
- "Rebuild" - Clean + Build

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

Currently no automated tests are configured. When adding tests:
- Consider using Qt Test framework
- Add test sources to separate directory
- Update .pro file with test configuration

## Additional Resources

- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [IRIG 106 Chapter 10 Standard](https://www.irig106.org/)
- [irig106utils GitHub](https://github.com/atac/irig106utils)
- [Qt Coding Conventions](https://wiki.qt.io/Qt_Coding_Style)

