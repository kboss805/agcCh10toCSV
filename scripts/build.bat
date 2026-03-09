@echo off
REM Out-of-source shadow build script
if not exist "..\build" mkdir "..\build"
cd "..\build"
C:\Qt\6.10.2\mingw_64\bin\qmake.exe ..\agcCh10toCSV.pro -spec win32-g++ CONFIG+=debug CONFIG+=qml_debug
if %errorlevel% neq 0 (
    echo QMake failed!
    cd ..\scripts
    exit /b %errorlevel%
)
C:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe -f Makefile.Debug
if %errorlevel% neq 0 (
    echo Make failed!
    cd ..\scripts
    exit /b %errorlevel%
)
cd ..\scripts
echo Build complete! Executable should be in the build/debug/ directory.
