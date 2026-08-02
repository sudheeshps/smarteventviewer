@echo off
setlocal enabledelayedexpansion

set SOLUTION_DIR=%~1
set CONFIG=%~2
set PLATFORM=%~3

:: Strip trailing backslash from SOLUTION_DIR if present to avoid xcopy quote escaping issues
if "%SOLUTION_DIR:~-1%"=="\" set SOLUTION_DIR=%SOLUTION_DIR:~0,-1%

if "%SOLUTION_DIR%"=="" set SOLUTION_DIR=%~dp0..
if "%CONFIG%"=="" set CONFIG=Release
if "%PLATFORM%"=="" set PLATFORM=x64

set DEPLOY_DIR=%SOLUTION_DIR%\bin\%PLATFORM%\%CONFIG%\IntegrationTestDeployment

echo =======================================================================
echo  Deploying SmartEventViewer Integration Test Environment...
echo  Target: %DEPLOY_DIR%
echo =======================================================================

if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"

echo [DEPLOY] Copying binaries...
xcopy "%SOLUTION_DIR%\bin\%PLATFORM%\%CONFIG%\*.exe" "%DEPLOY_DIR%\" /Y /Q /I >nul 2>&1
xcopy "%SOLUTION_DIR%\bin\%PLATFORM%\%CONFIG%\*.dll" "%DEPLOY_DIR%\" /Y /Q /I >nul 2>&1

echo [DEPLOY] Mirroring SmartEventViewerApp client components...
if not exist "%DEPLOY_DIR%\SmartEventViewerApp" mkdir "%DEPLOY_DIR%\SmartEventViewerApp"

if exist "%SOLUTION_DIR%\SmartEventViewerApp\package.json" (
    copy /Y "%SOLUTION_DIR%\SmartEventViewerApp\package.json" "%DEPLOY_DIR%\SmartEventViewerApp\" >nul 2>&1
)
if exist "%SOLUTION_DIR%\SmartEventViewerApp\vite.config.ts" (
    copy /Y "%SOLUTION_DIR%\SmartEventViewerApp\vite.config.ts" "%DEPLOY_DIR%\SmartEventViewerApp\" >nul 2>&1
)
if exist "%SOLUTION_DIR%\SmartEventViewerApp\tsconfig.json" (
    copy /Y "%SOLUTION_DIR%\SmartEventViewerApp\tsconfig*.json" "%DEPLOY_DIR%\SmartEventViewerApp\" >nul 2>&1
)

if exist "%SOLUTION_DIR%\SmartEventViewerApp\src" (
    if not exist "%DEPLOY_DIR%\SmartEventViewerApp\src" mkdir "%DEPLOY_DIR%\SmartEventViewerApp\src"
    xcopy "%SOLUTION_DIR%\SmartEventViewerApp\src" "%DEPLOY_DIR%\SmartEventViewerApp\src\" /S /E /Y /Q /I >nul 2>&1
)

if exist "%SOLUTION_DIR%\SmartEventViewerApp\node_modules" (
    if not exist "%DEPLOY_DIR%\SmartEventViewerApp\node_modules" mkdir "%DEPLOY_DIR%\SmartEventViewerApp\node_modules"
    xcopy "%SOLUTION_DIR%\SmartEventViewerApp\node_modules" "%DEPLOY_DIR%\SmartEventViewerApp\node_modules\" /S /E /Y /Q /I >nul 2>&1
)

echo [DEPLOY] Integration Test Environment deployed cleanly to %DEPLOY_DIR%!
exit /b 0
