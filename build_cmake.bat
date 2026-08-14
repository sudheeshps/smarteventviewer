@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - CMake Build Automation Script
echo ===================================================

set BUILD_TYPE=Release
set ACTION=build
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

echo [INFO] Building project targets...
cmake --build %BUILD_DIR% --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake build failed.
    exit /b %ERRORLEVEL%
)

echo [INFO] Copying latest React apiClient.ts to common output binary directory...
if exist SmartEventViewerApp\src\apiClient.ts (
    if not exist bin\x64\%BUILD_TYPE% mkdir bin\x64\%BUILD_TYPE%
    copy /y SmartEventViewerApp\src\apiClient.ts bin\x64\%BUILD_TYPE%\apiClient.ts
)

echo [INFO] Copying llama.cpp dependencies to output binary directory...
if exist vcpkg_installed\x64-windows\x64-windows\bin\llama.dll (
    if not exist bin\x64\%BUILD_TYPE% mkdir bin\x64\%BUILD_TYPE%
    copy /y vcpkg_installed\x64-windows\x64-windows\bin\*.dll bin\x64\%BUILD_TYPE%\
)

echo [INFO] Running Unit & Integration Test Suites...
ctest --test-dir %BUILD_DIR% -C %BUILD_TYPE% --output-on-failure
if %ERRORLEVEL% neq 0 (
    echo [WARNING] Tests finished with exit code %ERRORLEVEL%.
)

echo ===================================================
echo [SUCCESS] CMake %BUILD_TYPE% %ACTION% completed!
echo ===================================================
