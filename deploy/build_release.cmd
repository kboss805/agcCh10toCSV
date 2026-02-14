@echo off
setlocal enabledelayedexpansion

REM ====================================================================
REM  agcCh10toCSV Release Build & Package Script
REM  Builds a release binary, runs windeployqt, and creates both
REM  an Inno Setup installer staging layout and a portable ZIP.
REM
REM  Optional environment variables:
REM    SIGN_CERT_PATH  — path to .pfx code-signing certificate
REM    SIGN_CERT_PASS  — certificate password
REM    SIGN_TIMESTAMP  — timestamp server URL (default: http://timestamp.digicert.com)
REM ====================================================================

set VERSION=2.3.0
set PROJECT_DIR=%~dp0..
set QTDIR=C:\Qt\6.10.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%PATH%

set STAGE_DIR=%PROJECT_DIR%\deploy\staging
set INSTALLER_STAGE=%STAGE_DIR%\installer
set PORTABLE_STAGE=%STAGE_DIR%\portable
set PORTABLE_ROOT=%PORTABLE_STAGE%\agcCh10toCSV_v%VERSION%_portable

if not defined SIGN_TIMESTAMP set SIGN_TIMESTAMP=http://timestamp.digicert.com

echo ============================================
echo  Building agcCh10toCSV v%VERSION% Release
echo ============================================

REM --- Step 1: Clean and build release ---
cd /d "%PROJECT_DIR%"
echo [1/7] Running qmake...
qmake agcCh10toCSV.pro -spec win32-g++ "CONFIG+=release"
if %ERRORLEVEL% NEQ 0 (echo ERROR: qmake failed & exit /b 1)

echo [2/7] Building release...
mingw32-make -f Makefile.Release clean
mingw32-make -f Makefile.Release -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (echo ERROR: Build failed & exit /b 1)

REM --- Step 2: Prepare staging directories ---
echo [3/7] Preparing staging directories...
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%INSTALLER_STAGE%\bin"
mkdir "%INSTALLER_STAGE%\settings"
mkdir "%PORTABLE_ROOT%\settings"

REM --- Step 3: Copy exe and run windeployqt for INSTALLER layout ---
echo [4/7] Running windeployqt (installer layout)...
copy "%PROJECT_DIR%\release\agcCh10toCSV.exe" "%INSTALLER_STAGE%\bin\"
windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler "%INSTALLER_STAGE%\bin\agcCh10toCSV.exe"
if %ERRORLEVEL% NEQ 0 (echo ERROR: windeployqt failed & exit /b 1)

REM Copy settings to installer staging
copy "%PROJECT_DIR%\settings\default.ini" "%INSTALLER_STAGE%\settings\"
if exist "%PROJECT_DIR%\settings\RASA.ini" copy "%PROJECT_DIR%\settings\RASA.ini" "%INSTALLER_STAGE%\settings\"
if exist "%PROJECT_DIR%\settings\TRC.ini" copy "%PROJECT_DIR%\settings\TRC.ini" "%INSTALLER_STAGE%\settings\"

REM --- Step 4: Code signing (optional) ---
echo [5/7] Code signing...
if defined SIGN_CERT_PATH (
    signtool sign /f "%SIGN_CERT_PATH%" /p "%SIGN_CERT_PASS%" /tr "%SIGN_TIMESTAMP%" /td sha256 /fd sha256 "%INSTALLER_STAGE%\bin\agcCh10toCSV.exe"
    if %ERRORLEVEL% NEQ 0 (echo WARNING: Code signing failed — continuing without signature)
) else (
    echo  Skipping — set SIGN_CERT_PATH and SIGN_CERT_PASS to enable signing.
)

REM --- Step 5: Create PORTABLE layout (flat — exe at root) ---
echo [6/7] Creating portable layout...

REM Copy exe and all DLLs
copy "%INSTALLER_STAGE%\bin\agcCh10toCSV.exe" "%PORTABLE_ROOT%\"
for %%F in ("%INSTALLER_STAGE%\bin\*.dll") do copy "%%F" "%PORTABLE_ROOT%\"

REM Copy Qt plugin directories
if exist "%INSTALLER_STAGE%\bin\platforms" xcopy /s /i /q "%INSTALLER_STAGE%\bin\platforms" "%PORTABLE_ROOT%\platforms"
if exist "%INSTALLER_STAGE%\bin\styles" xcopy /s /i /q "%INSTALLER_STAGE%\bin\styles" "%PORTABLE_ROOT%\styles"
if exist "%INSTALLER_STAGE%\bin\imageformats" xcopy /s /i /q "%INSTALLER_STAGE%\bin\imageformats" "%PORTABLE_ROOT%\imageformats"
if exist "%INSTALLER_STAGE%\bin\tls" xcopy /s /i /q "%INSTALLER_STAGE%\bin\tls" "%PORTABLE_ROOT%\tls"
if exist "%INSTALLER_STAGE%\bin\networkinformation" xcopy /s /i /q "%INSTALLER_STAGE%\bin\networkinformation" "%PORTABLE_ROOT%\networkinformation"

REM Copy settings
copy "%PROJECT_DIR%\settings\default.ini" "%PORTABLE_ROOT%\settings\"
if exist "%PROJECT_DIR%\settings\RASA.ini" copy "%PROJECT_DIR%\settings\RASA.ini" "%PORTABLE_ROOT%\settings\"
if exist "%PROJECT_DIR%\settings\TRC.ini" copy "%PROJECT_DIR%\settings\TRC.ini" "%PORTABLE_ROOT%\settings\"

REM Create empty portable marker file
type nul > "%PORTABLE_ROOT%\portable"

REM --- Step 6: Create ZIP ---
echo [7/7] Creating portable ZIP...
powershell -Command "Compress-Archive -Path '%PORTABLE_ROOT%' -DestinationPath '%PROJECT_DIR%\deploy\agcCh10toCSV_v%VERSION%_portable.zip' -Force"
if %ERRORLEVEL% NEQ 0 (echo WARNING: ZIP creation failed)

echo.
echo ============================================
echo  Build and staging complete!
echo ============================================
echo.
echo  Installer staging: %INSTALLER_STAGE%
echo  Portable staging:  %PORTABLE_ROOT%
echo  Portable ZIP:      %PROJECT_DIR%\deploy\agcCh10toCSV_v%VERSION%_portable.zip
echo.
echo  Next: compile the installer with Inno Setup:
echo    iscc "%PROJECT_DIR%\deploy\agcCh10toCSV.iss"
echo ============================================
