@echo off
rem ===========================================================================
rem AICivilization run script
rem
rem Sets the working directory to the project root so that all relative paths
rem (logs\, data\, config\, assets\, cache\) resolve under D:\AICivilization,
rem then launches the built executable.
rem ===========================================================================
set "ROOT=%~dp0"
cd /d "%ROOT%"

if not exist "%ROOT%build\AICivilization.exe" (
    echo [run] ERROR: build\AICivilization.exe not found. Run build.bat first.
    exit /b 1
)

"%ROOT%build\AICivilization.exe" %*
