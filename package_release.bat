@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo SmartEventViewer - Release Deployment Packager
echo ===================================================

set DIST_DIR=dist\SmartEventViewer_v1.0.0_x64
set ZIP_NAME=dist\SmartEventViewer_v1.0.0_x64.zip

echo [INFO] Building Release Binaries via MSBuild...
call build_msbuild.bat release rebuild

if %ERRORLEVEL% neq 0 (
    echo [ERROR] MSBuild failed. Aborting deployment packaging.
    exit /b %ERRORLEVEL%
)

echo [INFO] Preparing Deployment Directory: %DIST_DIR%...
if exist dist (
    rmdir /s /q dist
)

mkdir "%DIST_DIR%\bin"
mkdir "%DIST_DIR%\UI"
mkdir "%DIST_DIR%\models"

echo [INFO] Copying Compiled Binaries ^& Dynamic Libraries from bin\x64\Release...
copy /y bin\x64\Release\*.dll "%DIST_DIR%\bin\" 2>nul
copy /y bin\x64\Release\*.lib "%DIST_DIR%\bin\" 2>nul
copy /y bin\x64\Release\SmartEventViewerServer.exe "%DIST_DIR%\bin\"
copy /y bin\x64\Release\SmartEventViewerTests.exe "%DIST_DIR%\bin\"
copy /y bin\x64\Release\SmartEventViewerIntegrationTests.exe "%DIST_DIR%\bin\" 2>nul

:: Dynamically detect the latest installed DotNetDupe package directory in packages\
set DOTNETDUPE_DIR=
for /d %%d in (packages\DotNetDupe.*) do (
    set DOTNETDUPE_DIR=%%d
)

if "%DOTNETDUPE_DIR%"=="" (
    echo [WARNING] DotNetDupe package folder not found in packages\. Using binaries from build output.
) else (
    echo [INFO] Copying Latest DotNetDupe Native Binaries from %DOTNETDUPE_DIR%...
    copy /y "%DOTNETDUPE_DIR%\runtimes\win-x64\native\DotNetDupe.dll" "%DIST_DIR%\bin\"
    copy /y "%DOTNETDUPE_DIR%\runtimes\win-x64\native\DotNetDupe.lib" "%DIST_DIR%\bin\"
    copy /y "%DOTNETDUPE_DIR%\runtimes\win-x64\native\libcrypto-4-x64.dll" "%DIST_DIR%\bin\"
    copy /y "%DOTNETDUPE_DIR%\runtimes\win-x64\native\libssl-4-x64.dll" "%DIST_DIR%\bin\"
)

echo [INFO] Building React + Vite SPA Frontend (SmartEventViewerApp)...
cd SmartEventViewerApp
call npm run build
cd ..

echo [INFO] Copying Front-end React + Vite Assets...
xcopy /s /e /y SmartEventViewerApp\dist "%DIST_DIR%\UI\"

echo [INFO] Copying Local GGUF LLM Model Files (models/Llama-3-8B-Instruct.Q4_K_M.gguf)...
if exist models (
    xcopy /s /e /y models "%DIST_DIR%\models\"
)

echo [INFO] Creating Launcher Script (start_smarteventviewer.bat)...
(
    echo @echo off
    echo echo Starting SmartEventViewer SIEM REST API Server and React Dashboard...
    echo start "" "bin\SmartEventViewerServer.exe" 8080
    echo timeout /t 2 /nobreak ^>nul
    echo start "" "http://localhost:8080/"
    echo echo SmartEventViewer is running at http://localhost:8080/
) > "%DIST_DIR%\start_smarteventviewer.bat"

echo ===================================================
echo [SUCCESS] Package created successfully in %DIST_DIR%!
echo ===================================================
