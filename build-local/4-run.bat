@echo off
rem Starts the built client with its own data folder, so it never shares
rem tdata with an installed official FAgram / Telegram Desktop.
rem Usage: 4-run.bat [Release^|Debug]   (default Release)
rem Set FA_DATA_DIR beforehand to use another data folder.

setlocal
set "FA_CONFIG=%~1"
if not defined FA_CONFIG set "FA_CONFIG=Release"

set "FA_EXE=%~dp0..\out\%FA_CONFIG%\fagram.exe"
if not exist "%FA_EXE%" (
    echo [run] %FA_EXE% not found, run 3-build.bat %FA_CONFIG% first.
    exit /b 1
)

if not defined FA_DATA_DIR set "FA_DATA_DIR=%~dp0..\out\data"
if not exist "%FA_DATA_DIR%" mkdir "%FA_DATA_DIR%"
for %%i in ("%FA_DATA_DIR%") do set "FA_DATA_DIR=%%~fi"

start "" "%FA_EXE%" -workdir "%FA_DATA_DIR%"
