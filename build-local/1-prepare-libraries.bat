@echo off
rem Builds all third-party libraries (Qt, ffmpeg, OpenSSL, WebRTC, ...) into
rem %FA_BUILD_ROOT%\Libraries\win64 and tools into %FA_BUILD_ROOT%\ThirdParty.
rem Takes several hours the first time; later runs only rebuild what changed.
rem Extra arguments are passed to prepare.py (for example a single stage name).

setlocal
call "%~dp0env.bat" || exit /b 1

call "%FA_REPO%\Telegram\build\prepare\win.bat" silent %*
exit /b %errorlevel%
