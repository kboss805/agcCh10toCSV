$env:QTDIR = 'C:\Qt\6.10.2\mingw_64'
$env:MINGW_DIR = 'C:\Qt\Tools\mingw1310_64'
$env:PATH = 'C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
Set-Location 'C:\Users\kevin\Dev\agcCH10toCSV\tests'
& "$env:MINGW_DIR\bin\mingw32-make.exe" -f Makefile.Debug
Write-Host "Tests=$LASTEXITCODE"
