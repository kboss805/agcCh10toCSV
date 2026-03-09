@echo off
REM ====================================================================
REM  Shared environment configuration for agcCh10toCSV build scripts.
REM  All paths are defined here so they only need to be updated in one place.
REM
REM  Each variable can be overridden by setting it before calling this script:
REM    set QTDIR=D:\Qt\6.10.2\mingw_64
REM    call scripts\env.bat
REM ====================================================================

if not defined QTDIR       set QTDIR=C:\Qt\6.10.2\mingw_64
if not defined MINGW_DIR   set MINGW_DIR=C:\Qt\Tools\mingw1310_64
if not defined WINSDK_BIN  set "WINSDK_BIN=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"

set PATH=%QTDIR%\bin;%MINGW_DIR%\bin;%WINSDK_BIN%;%PATH%
