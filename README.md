# Chapter 10 to CSV AGC Converter (agcCh10toCSV)

A desktop application for extracting, processing, and exporting multiplexed receiver Automatic Gain Control (AGC) signal samples from IRIG 106 Chapter 10 telemetry files. The exported data can be imported into 3rd party applications like Microsoft Excel and MATLAB for analyzing and plotting AGC signal samples.

## Overview

This application reads IRIG 106 Chapter 10 (.ch10) files, extracts telemetry data from specified channels, and exports AGC information to CSV format. It provides a user-friendly GUI for configuring data extraction parameters including time ranges, channel selection, and frame synchronization settings.

## Features

- **Chapter 10 File Support**: Read and parse IRIG 106 Chapter 10 telemetry files
- **Channel Selection**: Choose specific time and PCM channels for data extraction
- **Time Range Filtering**: Specify start and stop times (Day of Year, Hour, Minute, Second)
- **Frame Configuration**: Configure frame synchronization, randomization, and setup parameters
- **AGC Processing**: Extract and process Automatic Gain Control data with V-to-dB conversion
- **Receiver Selection**: Enable/disable specific receivers and channels per receiver
- **Settings Management**: Save and load processing configurations from INI files
- **CSV Export**: Output processed data in CSV format with auto-generated timestamped filenames
- **Dark & Light Themes**: Windows 11 / WinUI 3 styled dark and light themes with runtime toggle
- **Drag-and-Drop**: Drop .ch10 files directly onto the application window
- **Per-File-Type Directory Persistence**: Independently remembers the last used directory for Ch10, CSV, and INI file dialogs between sessions
- **Inline Log Window**: Persistent, scrollable log with timestamped entries; color-coded messages (green for success, yellow for warnings, red for errors); replaces modal dialog boxes for errors and warnings
- **Startup & File Logging**: Logs default.ini settings at startup, channel/time/frame info when opening Ch10 files, and INI validation details when loading settings
- **INI Validation**: Warns when parameter section count in INI files does not match receiver/channel configuration
- **Automatic Pre-Scan**: Detects PCM encoding and verifies frame sync on file open and PCM channel change
- **Sample Rate Options**: 1 Hz, 10 Hz, or 100 Hz output sample rates
- **Status Bar**: Displays file metadata summary (filename, size, channel counts, time range)
- **Settings Summary Panel**: Read-only display of current frame sync, polarity, slope, scale, and receiver configuration
- **Select All/None**: Quick receiver selection shortcuts in the receiver grid
- **Pre-Process Summary**: Logs input file, channels, time range, sample rate, receiver count, and output path before processing
- **Recent Files Menu**: File > Recent Files with up to 5 entries, persisted across sessions
- **Clickable Log Links**: Output file path and "Open Folder" links in the log window after processing completes
- **Batch Processing**: Multi-file selection and sequential queue-based batch processing with per-file channel selection, encoding detection, and color-coded status tracking
- **AGC Signal Plot Window**: Interactive QCustomPlot chart with mouse wheel zoom, click-drag pan, auto-scale axes, per-receiver-channel visibility toggles, and auto-assigned color palette
- **Plot PDF Export**: Export current plot to high-quality PDF file via QCustomPlot's built-in `savePdf()` method
- **X-Axis Time Display**: Actual file time (DDD:HH:MM:SS) on the X axis instead of elapsed seconds
- **Installer & Portable Distribution**: Inno Setup EXE installer with admin/non-admin support, portable ZIP with local settings, INI upgrade logic, and optional `.ch10` file association

## System Requirements

### Software
- **Qt**: Version 6.0.0 or later (developed on 6.10.2)
- **Compiler**: GCC/MinGW 7.0+ (developed on MinGW 13.1.0 64-bit)
- **C++ Standard**: C++17 required
- **Operating System**: Windows (primary), Linux/macOS (may require adjustments)

### Build Tools
- qmake (Qt build system)
- MinGW or compatible GCC toolchain

## Building the Project

### Using Qt Creator
1. Open `agcCh10toCSV.pro` in Qt Creator
2. Configure the project with your Qt kit
3. Build and run (Ctrl+R)

58: ### Using Command Line (Windows with MinGW)
59: ```bash
60: # We use a shadow build directory to keep the source tree clean
61: mkdir build
62: cd build
63: 
64: # Configure the project
65: qmake ../agcCh10toCSV.pro -spec win32-g++
66: 
67: # Build debug version
68: mingw32-make -f Makefile.Debug
69: 
70: # Build release version
71: mingw32-make -f Makefile.Release
72: 
73: # Run the application
74: debug\agcCH10toCSV.exe
75: cd ..
76: 
77: # Alternatively, use the provided build script
78: scripts\build.bat
79: ```


### Using VS Code
The project includes VS Code configuration files:
- `.vscode/tasks.json` - Build tasks
- `.vscode/launch.json` - Debug configurations
- `.vscode/c_cpp_properties.json` - C++ IntelliSense settings

To build and debug:
1. Open the project folder in VS Code
2. Press `Ctrl+Shift+B` to build
3. Press `F5` to debug

## Usage

1. **Load Input File**
   - Click the folder icon in the toolbar to browse for a Chapter 10 (.ch10) file
   - Or drag and drop a .ch10 file onto the application window

2. **Select Channels**
   - Choose a Time Channel from the dropdown
   - Choose a PCM Channel from the dropdown

3. **Configure Frame Settings**
   - Open Settings dialog from the File menu
   - Configure frame sync pattern, polarity, scale, and range
   - Or load settings from an INI configuration file

4. **Configure Receivers**
   - Select receivers and channels to include in the export
   - Use the tree checkboxes to enable/disable individual channels

5. **Set Time Range**
   - Check "All" to process entire file
   - Or specify start/stop times (DDD HH:MM:SS format)
   - Select sample rate (1 Hz, 10 Hz, or 100 Hz)

6. **Process**
   - Click the play icon in the toolbar to begin conversion
   - Choose output CSV file location
   - Monitor progress in the progress bar and log window

## Project Structure

```
agcCH10toCSV/
├── src/                        # Source files
│   ├── main.cpp               # Application entry point
│   ├── mainview.cpp           # Main GUI window (View)
│   ├── receivergridwidget.cpp # Receiver/channel selection grid (View)
│   ├── timeextractionwidget.cpp # Time range and sample rate controls (View)
│   ├── settingsdialog.cpp     # Settings dialog (View)
│   ├── mainviewmodel.cpp      # Application logic (ViewModel)
│   ├── chapter10reader.cpp    # Chapter 10 file metadata (Model)
│   ├── frameprocessor.cpp     # PCM frame extraction and CSV output (Model)
│   ├── framesetup.cpp         # Frame configuration parameters (Model)
│   ├── channeldata.cpp        # Channel metadata (Model)
│   ├── settingsmanager.cpp    # Settings persistence (Model)
│   ├── plotviewmodel.cpp      # Plot data parsing and axis management (ViewModel)
│   └── plotwidget.cpp         # QCustomPlot chart widget (View)
├── include/                    # Header files
│   ├── mainview.h
│   ├── receivergridwidget.h
│   ├── timeextractionwidget.h
│   ├── settingsdialog.h
│   ├── mainviewmodel.h
│   ├── chapter10reader.h
│   ├── frameprocessor.h
│   ├── framesetup.h
│   ├── channeldata.h
│   ├── settingsmanager.h
│   ├── settingsdata.h
│   ├── batchfileinfo.h
│   ├── plotviewmodel.h
│   ├── plotwidget.h
│   └── constants.h
├── lib/irig106/                # Third-party IRIG 106 library
│   ├── src/                   # irig106utils C source files
│   └── include/               # irig106utils C header files
├── lib/qcustomplot/            # Third-party QCustomPlot 2.1.1 charting library
├── deploy/                     # Build automation, installer, and release packaging
├── tests/                      # Qt Test framework unit tests
├── settings/                   # Default and user settings files
├── resources/                  # Resources (stylesheets, icons, rc)
│   ├── agcCh10toCSV_resource.rc # Windows resource file
│   ├── win11-dark.qss         # Dark theme stylesheet
│   ├── win11-light.qss        # Light theme stylesheet
│   ├── chevron-down-*.svg     # Combo box dropdown arrow icons
│   ├── checkmark.svg          # Checkbox checkmark icon
│   ├── folder-open.svg        # Toolbar open icon
│   ├── play.svg               # Toolbar process icon
│   └── icon.ico               # Application icon
├── scripts/                    # Build and utility scripts
│   ├── build.bat              # Command-line shadow build generator
│   └── commit.bat             # Git staging and commit helper
├── .vscode/                    # VS Code configuration
├── agcCh10toCSV.pro            # Qt project file
├── CLAUDE.md                   # AI assistant guide
└── README.md                   # This file
```

## Credits

This project incorporates code from the [irig106utils](https://github.com/atac/irig106utils) library for IRIG 106 Chapter 10 file handling and [QCustomPlot](https://www.qcustomplot.com/) 2.1.1 for interactive charting.

## License

GPL v3

## Contributing

[TBD]

## Support

Kevin
