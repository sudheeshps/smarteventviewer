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

:: Copy vcpkg dependencies (llama.dll, ggml.dll, etc.)
if exist vcpkg_installed\x64-windows\x64-windows\bin (
    copy /y vcpkg_installed\x64-windows\x64-windows\bin\*.dll "%DIST_DIR%\bin\" >nul 2>&1
)
if exist vcpkg_installed\x64-windows\bin (
    copy /y vcpkg_installed\x64-windows\bin\*.dll "%DIST_DIR%\bin\" >nul 2>&1
)

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

echo [INFO] Copying Front-end React + Vite Assets to UI and Package...
if not exist "%DIST_DIR%\UI" mkdir "%DIST_DIR%\UI"
if not exist "UI" mkdir "UI"
xcopy /s /e /y /i "SmartEventViewerApp\dist\*" "%DIST_DIR%\UI\"
xcopy /s /e /y /i "SmartEventViewerApp\dist\*" "UI\"

if exist "SmartEventViewerApp\public\favicon.ico" (
    copy /y "SmartEventViewerApp\public\favicon.ico" "%DIST_DIR%\UI\" >nul 2>&1
    copy /y "SmartEventViewerApp\public\favicon.ico" "UI\" >nul 2>&1
)

echo [INFO] Copying Local GGUF LLM Model Files (models/)...
if exist models (
    xcopy /s /e /y /i "models\*" "%DIST_DIR%\models\"
)

echo [INFO] Creating Launcher Script (start_smarteventviewer.bat)...
(
    echo @echo off
    echo setlocal enabledelayedexpansion
    echo cd /d "%%~dp0"
    echo echo Starting SmartEventViewer SIEM REST API Server and React Dashboard...
    echo start "" "bin\SmartEventViewerServer.exe" 8080
    echo timeout /t 2 /nobreak ^>nul
    echo start "" "http://127.0.0.1:8080/"
    echo echo SmartEventViewer is running at http://127.0.0.1:8080/
) > "%DIST_DIR%\start_smarteventviewer.bat"

rem echo [INFO] Creating ZIP Archive: %ZIP_NAME%...
rem powershell -NoProfile -Command "Compress-Archive -Path '%DIST_DIR%\*' -DestinationPath '%ZIP_NAME%' -Force"

echo ===================================================
echo [SUCCESS] Package created successfully in %DIST_DIR%!
echo [SUCCESS] Release ZIP archive: %ZIP_NAME%
echo ===================================================
