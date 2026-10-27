@echo off
rem Builds all third-party libraries (Qt, ffmpeg, OpenSSL, WebRTC, ...) into
rem %FA_BUILD_ROOT%\Libraries\win64 and tools into %FA_BUILD_ROOT%\ThirdParty.
rem Takes several hours the first time; later runs only rebuild what changed.
rem Extra arguments are passed to prepare.py (for example a single stage name).
rem A failed run is retried FA_RETRIES times, finished stages are not redone.

setlocal EnableDelayedExpansion
call "%~dp0env.bat" || exit /b 1

for /l %%a in (1,1,%FA_RETRIES%) do (
    echo [prepare] Attempt %%a of %FA_RETRIES%
    rem A gpg-agent left by a failed attempt locks files of the msys64 stage.
    powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -like '%FA_BUILD_ROOT%\ThirdParty\msys64\*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }"
    call "%FA_REPO%\Telegram\build\prepare\win.bat" silent %*
    if !errorlevel! equ 0 (
        echo [prepare] All libraries are ready.
        exit /b 0
    )
)
echo [prepare] Failed after %FA_RETRIES% attempts, see the output above.
exit /b 1
