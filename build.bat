@echo off
rem ===========================================================================
rem AICivilization build script (Windows / MSVC / Ninja)
rem
rem Locates Visual Studio with the C++ toolset via vswhere, imports the x64
rem developer environment (which puts the bundled cmake + ninja on PATH), then
rem configures and builds the project into the build\ directory.
rem ===========================================================================
setlocal enabledelayedexpansion
set "ROOT=%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] ERROR: vswhere.exe not found. Visual Studio with C++ tools is required.
    exit /b 1
)

set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [build] ERROR: No Visual Studio installation with the C++ toolset was found.
    exit /b 1
)
echo [build] Visual Studio: %VSPATH%

call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo [build] ERROR: Failed to initialize the Visual Studio developer environment.
    exit /b 1
)

echo [build] Configuring (Ninja, Release)...
cmake -S "%ROOT%." -B "%ROOT%build" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [build] ERROR: CMake configure failed.
    exit /b 1
)

echo [build] Building...
cmake --build "%ROOT%build"
if errorlevel 1 (
    echo [build] ERROR: Build failed.
    exit /b 1
)

echo [build] Done. Executable: %ROOT%build\AICivilization.exe
endlocal
