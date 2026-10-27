@echo off
rem Builds fagram.exe. Usage: 3-build.bat [Release^|Debug]   (default Release)

setlocal
call "%~dp0env.bat" || exit /b 1

set "FA_CONFIG=%~1"
if not defined FA_CONFIG set "FA_CONFIG=Release"

if not exist "%FA_REPO%\out\CMakeCache.txt" (
    echo [build] Not configured yet, run 2-configure.bat first.
    exit /b 1
)

cmake --build "%FA_REPO%\out" --config %FA_CONFIG% --target Telegram --parallel
exit /b %errorlevel%
