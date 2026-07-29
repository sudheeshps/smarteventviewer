@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - CMake Build Automation Script
echo ===================================================

set BUILD_TYPE=Release
set ACTION=build

:parse_args
if "%~1"=="" goto end_parse
if /i "%~1"=="debug" (
    set BUILD_TYPE=Debug
) else if /i "%~1"=="release" (
    set BUILD_TYPE=Release
) else if /i "%~1"=="clean" (
    set ACTION=clean
) else if /i "%~1"=="rebuild" (
    set ACTION=rebuild
)
shift
goto parse_args

:end_parse

set BUILD_DIR=build_%BUILD_TYPE%

if "%ACTION%"=="clean" goto do_clean
if "%ACTION%"=="rebuild" (
    call :do_clean
    goto do_build
)
if "%ACTION%"=="build" goto do_build

:do_clean
echo [INFO] Cleaning CMake build directory: %BUILD_DIR%...
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)
if "%ACTION%"=="clean" (
    echo [SUCCESS] Clean completed.
    exit /b 0
)
goto :eof

:do_build
echo [INFO] Configuring CMake (%BUILD_TYPE% build)...
cmake -B %BUILD_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Building SmartEventViewer targets (%BUILD_TYPE%)...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake build failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Running CTest Unit Tests...
ctest --test-dir %BUILD_DIR% -C %BUILD_TYPE% --output-on-failure

echo ===================================================
echo [SUCCESS] CMake %BUILD_TYPE% %ACTION% completed!
echo ===================================================
exit /b 0
