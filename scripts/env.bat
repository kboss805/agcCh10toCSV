@echo off
REM ====================================================================
REM  Shared environment configuration for agcCh10toCSV build scripts.
REM  QTDIR and MINGW_DIR are the single source of truth for all tool paths.
REM
REM  These same variable names are used in:
REM    .vscode\launch.json        (${env:QTDIR}, ${env:MINGW_DIR})
REM    .vscode\settings.json      (${env:MINGW_DIR})
REM    .vscode\c_cpp_properties.json  (${env:QTDIR}, ${env:MINGW_DIR})
REM
REM  For VS Code to expand them, register them as Windows user environment
REM  variables once by running:  powershell -File scripts\setup-env.ps1
REM
REM  Each variable can be overridden by setting it before calling this script:
REM    set QTDIR=D:\Qt\6.10.2\mingw_64
REM    call scripts\env.bat
REM ====================================================================

if not defined QTDIR       set QTDIR=C:\Qt\6.10.2\mingw_64
if not defined MINGW_DIR   set MINGW_DIR=C:\Qt\Tools\mingw1310_64
if not defined WINSDK_BIN  set "WINSDK_BIN=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"

set PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%WINSDK_BIN%;%PATH%
