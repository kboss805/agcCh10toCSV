@echo off
setlocal enabledelayedexpansion

REM ====================================================================
REM  agcCh10toCSV Release Build & Package Script
REM  Builds a release binary, runs windeployqt, and creates both
REM  an Inno Setup installer staging layout and a portable ZIP.
REM
REM  Optional environment variables:
REM    SIGN_CERT_SHA1  — SHA-1 thumbprint of the code-signing certificate
REM                      in the Windows Certificate Store (CurrentUser\My)
REM                      Current cert: 1DCBF23B52067A8D52E9C517345EA77B9D926669
REM    SIGN_TIMESTAMP  — timestamp server URL (default: http://timestamp.digicert.com)
REM ====================================================================

set VERSION=2.3.0
set PROJECT_DIR=%~dp0..
set QTDIR=C:\Qt\6.10.2\mingw_64
set MINGW_DIR=C:\Qt\Tools\mingw1310_64
set WINSDK_BIN=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64
set PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%WINSDK_BIN%;%PATH%

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
echo [5/8] Code signing...
if defined SIGN_CERT_SHA1 (
    signtool sign /sha1 %SIGN_CERT_SHA1% /tr %SIGN_TIMESTAMP% /td sha256 /fd sha256 "%INSTALLER_STAGE%\bin\agcCh10toCSV.exe"
    if %ERRORLEVEL% NEQ 0 (echo WARNING: Code signing failed — continuing without signature)
) else (
    echo  Skipping — set SIGN_CERT_SHA1 to enable signing.
)

REM --- Step 5: Create PORTABLE layout (flat — exe at root) ---
echo [6/8] Creating portable layout...

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
echo [7/8] Creating portable ZIP...
powershell -Command "Compress-Archive -Path '%PORTABLE_ROOT%' -DestinationPath '%PROJECT_DIR%\deploy\agcCh10toCSV_v%VERSION%_portable.zip' -Force"
if %ERRORLEVEL% NEQ 0 (echo WARNING: ZIP creation failed)

REM --- Step 7: Compile Inno Setup installer ---
echo [8/8] Compiling Inno Setup installer...
set ISCC_PATH=
where iscc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set ISCC_PATH=iscc
) else if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" (
    set "ISCC_PATH=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
) else if exist "%PROGRAMFILES(x86)%\Inno Setup 6\ISCC.exe" (
    set "ISCC_PATH=%PROGRAMFILES(x86)%\Inno Setup 6\ISCC.exe"
)

if defined ISCC_PATH (
    if defined SIGN_CERT_SHA1 (
        "%ISCC_PATH%" /DSIGN /Ssigntool="signtool.exe sign /sha1 %SIGN_CERT_SHA1% /tr %SIGN_TIMESTAMP% /td sha256 /fd sha256 $f" "%PROJECT_DIR%\deploy\agcCh10toCSV.iss"
    ) else (
        "%ISCC_PATH%" "%PROJECT_DIR%\deploy\agcCh10toCSV.iss"
    )
    if %ERRORLEVEL% NEQ 0 (echo WARNING: Inno Setup compilation failed)
) else (
    echo  Skipping — Inno Setup not found. Install from https://jrsoftware.org/isinfo.php
    echo  Then run: iscc "%PROJECT_DIR%\deploy\agcCh10toCSV.iss"
)

echo.
echo ============================================
echo  Build and packaging complete!
echo ============================================
echo.
echo  Installer staging: %INSTALLER_STAGE%
echo  Portable staging:  %PORTABLE_ROOT%
echo  Portable ZIP:      %PROJECT_DIR%\deploy\agcCh10toCSV_v%VERSION%_portable.zip
echo  Installer EXE:     %PROJECT_DIR%\deploy\agcCh10toCSV_v%VERSION%_setup.exe
echo ============================================
