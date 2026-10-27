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

set "FA_ENV_READY=1"
echo [env] Visual Studio: %FA_VSDIR%
echo [env] Repository:    %FA_REPO%
echo [env] Build root:    %FA_BUILD_ROOT%
exit /b 0
