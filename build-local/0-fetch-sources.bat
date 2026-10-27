@echo off
rem Checks out every git submodule at the commit pinned by the repository.
rem Interrupted clones are resumed, the whole step is retried FA_RETRIES times.

setlocal EnableDelayedExpansion
call "%~dp0env.bat" || exit /b 1

cd /d "%FA_REPO%"
for /l %%a in (1,1,%FA_RETRIES%) do (
    echo [sources] Attempt %%a of %FA_RETRIES%
    git submodule sync --recursive >nul
    git submodule update --init --recursive --jobs 4
    if !errorlevel! equ 0 (
        echo [sources] All submodules are checked out.
        exit /b 0
    )
)
echo [sources] Failed after %FA_RETRIES% attempts, see the output above.
exit /b 1
