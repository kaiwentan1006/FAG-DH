@echo off
rem Generates the Visual Studio solution in %FA_REPO%\out.
rem Reads api_id / api_hash from api_credentials.txt next to this script.
rem Extra arguments are passed to Telegram\configure.bat as CMake options.

setlocal
call "%~dp0env.bat" || exit /b 1

set "FA_CREDENTIALS=%~dp0api_credentials.txt"
if not exist "%FA_CREDENTIALS%" (
    echo [configure] %FA_CREDENTIALS% not found.
    echo [configure] Copy api_credentials.example.txt to api_credentials.txt and fill it in.
    exit /b 1
)
set "FA_api_id="
set "FA_api_hash="
for /f "usebackq eol=# tokens=1,* delims==" %%a in ("%FA_CREDENTIALS%") do set "FA_%%a=%%b"
if not defined FA_api_id goto bad_credentials
if not defined FA_api_hash goto bad_credentials
if "%FA_api_id%"=="0" goto bad_credentials

rem Auto update is off: an update would replace this build with the official one.
rem Crash reports are off: they would be sent to the official FAgram server.
call "%FA_REPO%\Telegram\configure.bat" x64 ^
    -D TDESKTOP_API_ID=%FA_api_id% ^
    -D TDESKTOP_API_HASH=%FA_api_hash% ^
    -D DESKTOP_APP_DISABLE_AUTOUPDATE=ON ^
    -D DESKTOP_APP_DISABLE_CRASH_REPORTS=ON ^
    %*
exit /b %errorlevel%

:bad_credentials
echo [configure] api_id / api_hash are missing in %FA_CREDENTIALS%.
exit /b 1
