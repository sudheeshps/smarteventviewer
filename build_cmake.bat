@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - CMake Build Automation Script
echo ===================================================

set BUILD_TYPE=Release
set ACTION=build
set RUN_TESTS=0
set MODEL_DIR=models
set MODEL_FILE=%MODEL_DIR%\Qwen1.5-4B-Chat-Q4_K_M.gguf
set MODEL_URL=https://huggingface.co/Qwen/Qwen1.5-4B-Chat-GGUF/resolve/main/qwen1_5-4b-chat-q4_k_m.gguf?download=true

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
) else if /i "%~1"=="test" (
    set RUN_TESTS=1
)
shift
goto parse_args

:end_parse

:: Check and ensure local LLM model file exists
if not exist "%MODEL_DIR%" (
    echo [INFO] Creating missing models directory at .\%MODEL_DIR%
    mkdir "%MODEL_DIR%"
)

if not exist "%MODEL_FILE%" (
    echo [INFO] Local LLM model file not found at .\%MODEL_FILE%.
    set /p USER_CHOICE="Do you want to download the local GGUF model binary file (~1 GB) now? (Y/N): "
    if /i "!USER_CHOICE!"=="Y" (
        echo [INFO] Downloading quantized model file...
        powershell -Command "Invoke-WebRequest -Uri '%MODEL_URL%' -OutFile '%MODEL_FILE%'"
        if exist "%MODEL_FILE%" (
            echo [SUCCESS] Local LLM model file downloaded successfully to .\%MODEL_FILE%!
        ) else (
            echo [WARNING] Automatic model download failed. You may manually place a GGUF model file into .\%MODEL_FILE%.
        )
    ) else (
        echo [INFO] Model download skipped by user. Build will proceed without model download.
    )
) else (
    echo [INFO] Verified local LLM model binary exists at .\%MODEL_FILE%.
)

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
cmake -B %BUILD_DIR% -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Building project targets (Core, Server, Tests, IntegrationTests)...
cmake --build %BUILD_DIR% --config %BUILD_TYPE% --target SmartEventViewerCore SmartEventViewerServer SmartEventViewerTests SmartEventViewerIntegrationTests
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake build failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Copying runtime dependencies to binary directories...
if not exist bin\x64\%BUILD_TYPE% mkdir bin\x64\%BUILD_TYPE%
if not exist %BUILD_DIR%\bin\%BUILD_TYPE% mkdir %BUILD_DIR%\bin\%BUILD_TYPE%

if exist packages\DotNetDupe.4.0.0\runtimes\win-x64\native (
    copy /y packages\DotNetDupe.4.0.0\runtimes\win-x64\native\*.dll bin\x64\%BUILD_TYPE%\ >nul 2>&1
    copy /y packages\DotNetDupe.4.0.0\runtimes\win-x64\native\*.dll %BUILD_DIR%\bin\%BUILD_TYPE%\ >nul 2>&1
)

if exist vcpkg_installed\x64-windows\x64-windows\bin (
    copy /y vcpkg_installed\x64-windows\x64-windows\bin\*.dll bin\x64\%BUILD_TYPE%\ >nul 2>&1
    copy /y vcpkg_installed\x64-windows\x64-windows\bin\*.dll %BUILD_DIR%\bin\%BUILD_TYPE%\ >nul 2>&1
)


if %RUN_TESTS% equ 1 (
    echo [INFO] Running Unit and Integration Test Suites via CTest...
    ctest --test-dir %BUILD_DIR% -C %BUILD_TYPE% --output-on-failure
    if %ERRORLEVEL% neq 0 (
        echo [WARNING] CTest finished with exit code %ERRORLEVEL%.
    )
)

echo ===================================================
echo [SUCCESS] CMake %BUILD_TYPE% %ACTION% completed!
echo ===================================================
