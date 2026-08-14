@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - MSBuild Automation Script
echo ===================================================

set CONFIG=Release
set PLATFORM=x64
set TARGET=Build
set MODEL_DIR=models
set MODEL_FILE=%MODEL_DIR%\Qwen1.5-4B-Chat-Q4_K_M.gguf
set MODEL_URL=https://huggingface.co/Qwen/Qwen1.5-4B-Chat-GGUF/resolve/main/qwen1_5-4b-chat-q4_k_m.gguf?download=true

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

echo [INFO] Running MSBuild (Configuration=%CONFIG%, Platform=%PLATFORM%, Target=%TARGET%)...
msbuild SmartEventViewer.sln /t:%TARGET% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] MSBuild failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

echo [INFO] Copying latest React apiClient.ts to common output binary directory...
if exist SmartEventViewerApp\src\apiClient.ts (
    copy /y SmartEventViewerApp\src\apiClient.ts bin\%PLATFORM%\%CONFIG%\apiClient.ts
)

echo [INFO] Copying llama.cpp dependencies to output binary directory...
if exist vcpkg_installed\x64-windows\x64-windows\bin\llama.dll (
    copy /y vcpkg_installed\x64-windows\x64-windows\bin\*.dll bin\%PLATFORM%\%CONFIG%\
)

if "%TARGET%"=="Clean" (
    echo [SUCCESS] MSBuild Clean completed successfully.
    exit /b 0
)

rem echo [INFO] Running Unit Test Suite...
rem if exist .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerTests.exe (
rem    .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerTests.exe
rem )

rem echo [INFO] Running Integration Test Suite...
rem if exist .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerIntegrationTests.exe (
rem    .\bin\%PLATFORM%\%CONFIG%\SmartEventViewerIntegrationTests.exe
rem )

echo ===================================================
echo [SUCCESS] MSBuild %CONFIG% %TARGET% completed!
echo ===================================================
