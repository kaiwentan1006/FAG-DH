@echo off
rem Loads the x64 MSVC toolchain of Visual Studio 2022 into the current shell.
rem Every other script in this folder calls this one first.

if defined FA_ENV_READY exit /b 0

set "FA_REPO=%~dp0.."
for %%i in ("%FA_REPO%") do set "FA_REPO=%%~fi"
for %%i in ("%FA_REPO%\..") do set "FA_BUILD_ROOT=%%~fi"

set "FA_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%FA_VSWHERE%" (
    echo [env] vswhere.exe not found, install Visual Studio 2022 first.
    exit /b 1
)

set "FA_VSDIR="
for /f "usebackq tokens=*" %%i in (`"%FA_VSWHERE%" -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -latest -property installationPath`) do set "FA_VSDIR=%%i"
if not defined FA_VSDIR (
    echo [env] Visual Studio 2022 with "Desktop development with C++" not found.
    exit /b 1
)

call "%FA_VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [env] vcvars64.bat failed.
    exit /b 1
)

rem gyp based stages (breakpad) look Visual Studio up through these.
set "GYP_MSVS_OVERRIDE_PATH=%FA_VSDIR%\"
set "GYP_MSVS_VERSION=2022"
set "PYTHONUNBUFFERED=1"

rem Long transfers from GitHub / googlesource get dropped on some networks,
rem so the download steps rerun this many times, resuming where they stopped.
rem Machine specific overrides (FA_PROXY, FA_RETRIES, ...), not committed.
if exist "%~dp0local-settings.bat" call "%~dp0local-settings.bat"

rem Optional proxy for every download (git, pacman, python, curl).
if defined FA_PROXY (
    set "HTTP_PROXY=%FA_PROXY%"
    set "HTTPS_PROXY=%FA_PROXY%"
    set "ALL_PROXY=%FA_PROXY%"
    set "NO_PROXY=localhost,127.0.0.1"
)

if not defined FA_RETRIES set "FA_RETRIES=8"

rem MSYS2 package mirrors tried before the official ones (space separated,
rem "none" to use only the official list). Defaults are mainland China ones.
if not defined FA_MSYS2_MIRRORS set "FA_MSYS2_MIRRORS=https://mirrors.tuna.tsinghua.edu.cn/msys2 https://mirrors.aliyun.com/msys2"

rem Git settings for every clone made by these scripts, without touching the
rem global git config: long paths, abort stalled transfers instead of hanging.
set "GIT_CONFIG_COUNT=4"
set "GIT_CONFIG_KEY_0=core.longpaths"
set "GIT_CONFIG_VALUE_0=true"
set "GIT_CONFIG_KEY_1=http.version"
set "GIT_CONFIG_VALUE_1=HTTP/1.1"
set "GIT_CONFIG_KEY_2=http.lowSpeedLimit"
set "GIT_CONFIG_VALUE_2=1000"
set "GIT_CONFIG_KEY_3=http.lowSpeedTime"
set "GIT_CONFIG_VALUE_3=60"

set "FA_ENV_READY=1"
echo [env] Visual Studio: %FA_VSDIR%
echo [env] Repository:    %FA_REPO%
echo [env] Build root:    %FA_BUILD_ROOT%
if defined FA_PROXY (echo [env] Proxy:         %FA_PROXY%) else (echo [env] Proxy:         none)
exit /b 0
