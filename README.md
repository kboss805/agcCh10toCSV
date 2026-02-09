# Chapter 10 to CSV AGC Converter (agcCh10toCSV)

A desktop application for extracting, processing, exporting multiplexted receiver Automatic Gain Control (AGC) signal samples in IRIG 106 Chapter 10 telemetry files. The exported can be imported into 3rd party applications like Microsoft Excel and MATLAB for analyzing and plotting the AGC signal samples.

## Overview

This application reads IRIG 106 Chapter 10 (.ch10) files, extracts telemetry data from specified channels, and exports AGC information to CSV format. It provides a user-friendly GUI for configuring data extraction parameters including time ranges, channel selection, and frame synchronization settings.

## Features

- **Chapter 10 File Support**: Read and parse IRIG 106 Chapter 10 telemetry files
- **Channel Selection**: Choose specific time and PCM channels for data extraction
- **Time Range Filtering**: Specify start and stop times (Day of Year, Hour, Minute, Second)
- **Frame Configuration**: Configure frame synchronization, randomization, and setup parameters
- **AGC Processing**: Extract and process Automatic Gain Control data
- **Receiver Selection**: Enable/disable specific receivers (1-16)
- **Settings Management**: Save and load processing configurations
- **CSV Export**: Output processed data in CSV format
- **Dark Theme UI**: Modern dark theme interface (win11-dark.qss)

## System Requirements

### Software
- **Qt**: Version 6.10.2 or later
- **Compiler**: MinGW 13.1.0 (64-bit) or compatible C++11 compiler
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
   - Click "..." to browse for a Chapter 10 (.ch10) file
   - The application will parse the file and populate available channels

2. **Select Channels**
   - Choose a Time Channel from the dropdown
   - Choose a PCM Channel from the dropdown

3. **Configure Frame Settings**
   - Select randomization option
   - Enter frame sync pattern (hexadecimal)
   - Load frame setup file if needed

4. **Set Parameters**
   - Check "Use AGC info from Frame Setup File" to use default settings
   - Or configure polarity, slope, and scale manually

5. **Configure Receivers**
   - Select "All" to enable all receivers
   - Or individually select receivers 1-16

6. **Set Time Range**
   - Check "All" to process entire file
   - Or specify start/stop times (DDD HH:MM:SS format)
   - Select sample rate and time format

7. **Specify Output**
   - Enter output CSV file path

8. **Process**
   - Click "Process" to begin conversion
   - Monitor progress and status

## Project Structure

```
agcCH10toCSV/
├── src/                    # Source files
│   ├── main.cpp           # Application entry point
│   ├── mainwindow.cpp     # Main GUI window
│   ├── chapter10reader.cpp # Chapter 10 file parser
│   ├── settingsmanager.cpp # Settings persistence
│   └── irig106*.c         # IRIG 106 library files
├── include/               # Header files
│   ├── mainwindow.h
│   ├── chapter10reader.h
│   └── irig106ch10.h
├── resources/             # Resources (stylesheets, icons)
│   └── win11-dark.qss    # Dark theme stylesheet
├── .vscode/              # VS Code configuration
├── agcCh10toCSV.pro               # Qt project file
└── README.md             # This file
```

## Credits

This project incorporates code from the [irig106utils](https://github.com/atac/irig106utils) library for IRIG 106 Chapter 10 file handling.

## License

GPL v3

## Contributing

[TBD]

## Support

Kevin
