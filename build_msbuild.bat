@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - MSBuild Automation Script
echo ===================================================

set CONFIG=Release
set PLATFORM=x64
set TARGET=Build

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="debug" (
    set CONFIG=Debug
) else if /i "%~1"=="release" (
    set CONFIG=Release
) else if /i "%~1"=="clean" (
    set TARGET=Clean
) else if /i "%~1"=="rebuild" (
    set TARGET=Rebuild
)
shift
goto parse_args

:end_parse

echo [INFO] Running MSBuild (Configuration=%CONFIG%, Platform=%PLATFORM%, Target=%TARGET%)...
msbuild SmartEventViewer.sln /t:%TARGET% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] MSBuild failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

if "%TARGET%"=="Clean" (
    echo [SUCCESS] MSBuild Clean completed successfully.
    exit /b 0
)

echo [INFO] Running Unit Test Suite...
if exist .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerTests.exe (
    .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerTests.exe
) else (
    echo [WARNING] Test runner binary not found at .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerTests.exe
)

echo ===================================================
echo [SUCCESS] MSBuild %CONFIG% %TARGET% completed!
echo ===================================================
exit /b 0
