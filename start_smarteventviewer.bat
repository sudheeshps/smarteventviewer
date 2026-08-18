@echo off
setlocal

set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo =======================================================================
echo  Starting SmartEventViewer SIEM REST API Server and Dashboard UI...
echo =======================================================================

:: Locate SmartEventViewerServer.exe binary
set SERVER_EXE=
if exist "bin\x64\Release\SmartEventViewerServer.exe" set SERVER_EXE=bin\x64\Release\SmartEventViewerServer.exe
if exist "bin\Release\SmartEventViewerServer.exe" set SERVER_EXE=bin\Release\SmartEventViewerServer.exe
if exist "bin\x64\Debug\SmartEventViewerServer.exe" set SERVER_EXE=bin\x64\Debug\SmartEventViewerServer.exe
if exist "bin\SmartEventViewerServer.exe" set SERVER_EXE=bin\SmartEventViewerServer.exe

if "%SERVER_EXE%"=="" (
    echo [ERROR] Could not find SmartEventViewerServer.exe in bin directory.
    echo Please build the project first using build_msbuild.bat or build_cmake.bat.
    pause
    exit /b 1
)

:: Self-elevate batch launcher to Administrator if running as standard user
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ELEVATION REQUIRED] Requesting Administrator privileges to access Windows Security Event Logs...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo [LAUNCH] Starting Native C++ REST API Server: %SERVER_EXE%
start "" "%SERVER_EXE%" 8080

echo [WAIT] Waiting 2 seconds for server to initialize on http://127.0.0.1:8080/ ...
timeout /t 2 /nobreak >nul

echo [BROWSER] Opening SIEM Dashboard at http://127.0.0.1:8080/ ...
start "" "http://127.0.0.1:8080/"

echo.
echo SmartEventViewer SIEM is running!
