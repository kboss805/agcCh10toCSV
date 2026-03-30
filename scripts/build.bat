@echo off
REM In-source debug build script
call "%~dp0env.bat"

cd /d "%~dp0.."
qmake.exe agcCh10toCSV.pro -spec win32-g++ CONFIG+=debug CONFIG+=qml_debug
if %errorlevel% neq 0 (
    echo QMake failed!
    exit /b %errorlevel%
)
mingw32-make.exe -f Makefile.Debug
if %errorlevel% neq 0 (
    echo Make failed!
    exit /b %errorlevel%
)
echo Build complete! Executable is in debug\agcCH10toCSV.exe
