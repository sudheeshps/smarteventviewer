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

echo [INFO] Copying Native Binaries ^& Libraries...
copy /y bin\x64\Release\SmartEventViewer.dll "%DIST_DIR%\bin\"
copy /y bin\x64\Release\SmartEventViewer.lib "%DIST_DIR%\bin\"
copy /y bin\x64\Release\SmartEventViewerServer.exe "%DIST_DIR%\bin\"
copy /y bin\x64\Release\SmartEventViewerTests.exe "%DIST_DIR%\bin\"

echo [INFO] Building React + Vite SPA Frontend (ui-app)...
cd ui-app
call npm run build
cd ..

echo [INFO] Copying Front-end React + Vite Assets...
xcopy /s /e /y ui-app\dist "%DIST_DIR%\UI\"

echo [INFO] Copying Local GGUF LLM Model Files...
if exist models (
    xcopy /s /e /y models "%DIST_DIR%\models\"
)

echo [INFO] Creating Launcher Script (start_smarteventviewer.bat)...
(
    echo @echo off
    echo echo Starting SmartEventViewer SIEM REST API Server ^& React Dashboard...
    echo start "" "bin\SmartEventViewerServer.exe" 8080
    echo timeout /t 2 /nobreak ^>nul
    echo start "" "http://localhost:8080/"
    echo echo SmartEventViewer is running at http://localhost:8080/
) > "%DIST_DIR%\start_smarteventviewer.bat"

echo [INFO] Creating Release ZIP Archive (%ZIP_NAME%)...
powershell -Command "Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath '%ZIP_NAME%' -Force"

if exist "%ZIP_NAME%" (
    echo ===================================================
    echo [SUCCESS] Package created successfully!
    echo Zip File: %ZIP_NAME%
    echo Directory: %DIST_DIR%
    echo ===================================================
) else (
    echo [ERROR] Failed to generate zip archive.
)
