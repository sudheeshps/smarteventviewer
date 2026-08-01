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

echo [DEPLOY] Mirroring ui-app client components...
if not exist "%DEPLOY_DIR%\ui-app" mkdir "%DEPLOY_DIR%\ui-app"

if exist "%SOLUTION_DIR%\ui-app\package.json" (
    copy /Y "%SOLUTION_DIR%\ui-app\package.json" "%DEPLOY_DIR%\ui-app\" >nul 2>&1
)
if exist "%SOLUTION_DIR%\ui-app\vite.config.ts" (
    copy /Y "%SOLUTION_DIR%\ui-app\vite.config.ts" "%DEPLOY_DIR%\ui-app\" >nul 2>&1
)
if exist "%SOLUTION_DIR%\ui-app\tsconfig.json" (
    copy /Y "%SOLUTION_DIR%\ui-app\tsconfig*.json" "%DEPLOY_DIR%\ui-app\" >nul 2>&1
)

if exist "%SOLUTION_DIR%\ui-app\src" (
    if not exist "%DEPLOY_DIR%\ui-app\src" mkdir "%DEPLOY_DIR%\ui-app\src"
    xcopy "%SOLUTION_DIR%\ui-app\src" "%DEPLOY_DIR%\ui-app\src\" /S /E /Y /Q /I >nul 2>&1
)

if exist "%SOLUTION_DIR%\ui-app\node_modules" (
    if not exist "%DEPLOY_DIR%\ui-app\node_modules" mkdir "%DEPLOY_DIR%\ui-app\node_modules"
    xcopy "%SOLUTION_DIR%\ui-app\node_modules" "%DEPLOY_DIR%\ui-app\node_modules\" /S /E /Y /Q /I >nul 2>&1
)

echo [DEPLOY] Integration Test Environment deployed cleanly to %DEPLOY_DIR%!
exit /b 0
