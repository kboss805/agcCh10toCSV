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
- **Inline Log Window**: Persistent, scrollable log with timestamped entries; errors in red, warnings in dark yellow; replaces modal dialog boxes for errors and warnings
- **Startup & File Logging**: Logs default.ini settings at startup, channel/time/frame info when opening Ch10 files, and INI validation details when loading settings
- **INI Validation**: Warns when parameter section count in INI files does not match receiver/channel configuration
- **Sample Rate Options**: 1 Hz, 10 Hz, or 100 Hz output sample rates

## System Requirements

### Software
- **Qt**: Version 6.10.2 or later
- **Compiler**: MinGW 13.1.0 (64-bit) or compatible C++17 compiler
- **Operating System**: Windows (primary), Linux/macOS (may require adjustments)

### Build Tools
- qmake (Qt build system)
- MinGW or compatible GCC toolchain

## Building the Project

### Using Qt Creator
1. Open `agcCh10toCSV.pro` in Qt Creator
2. Configure the project with your Qt kit
3. Build and run (Ctrl+R)

### Using Command Line (Windows with MinGW)
```bash
# Configure the project
qmake agcCh10toCSV.pro -spec win32-g++

# Build debug version
mingw32-make -f Makefile.Debug

# Build release version
mingw32-make -f Makefile.Release

# Run the application
debug\agcCh10toCSV.exe
```

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
   - Select sample rate (1 Hz, 10 Hz, or 20 Hz)

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
│   ├── settingsdialog.cpp     # Settings dialog (View)
│   ├── mainviewmodel.cpp      # Application logic (ViewModel)
│   ├── chapter10reader.cpp    # Chapter 10 file metadata (Model)
│   ├── frameprocessor.cpp     # PCM frame extraction and CSV output (Model)
│   ├── framesetup.cpp         # Frame configuration parameters (Model)
│   ├── channeldata.cpp        # Channel metadata (Model)
│   └── settingsmanager.cpp    # Settings persistence (Model)
├── include/                    # Header files
│   ├── mainview.h
│   ├── settingsdialog.h
│   ├── mainviewmodel.h
│   ├── chapter10reader.h
│   ├── frameprocessor.h
│   ├── framesetup.h
│   ├── channeldata.h
│   ├── settingsmanager.h
│   ├── settingsdata.h
│   └── constants.h
├── lib/irig106/                # Third-party IRIG 106 library
│   ├── src/                   # irig106utils C source files
│   └── include/               # irig106utils C header files
├── tests/                      # Qt Test framework unit tests
├── settings/                   # Default and user settings files
├── resources/                  # Resources (stylesheets, icons)
│   ├── win11-dark.qss         # Dark theme stylesheet
│   ├── win11-light.qss        # Light theme stylesheet
│   ├── chevron-down-*.svg     # Combo box dropdown arrow icons
│   ├── checkmark.svg          # Checkbox checkmark icon
│   ├── folder-open.svg        # Toolbar open icon
│   ├── play.svg               # Toolbar process icon
│   └── icon.ico               # Application icon
├── .vscode/                    # VS Code configuration
├── agcCh10toCSV.pro            # Qt project file
├── CLAUDE.md                   # AI assistant guide
└── README.md                   # This file
```

## Credits

This project incorporates code from the [irig106utils](https://github.com/atac/irig106utils) library for IRIG 106 Chapter 10 file handling.

## License

GPL v3

## Contributing

[TBD]

## Support

Kevin
