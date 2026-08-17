@echo off
setlocal
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    set MSBUILD_EXE=%%i
)

if "%MSBUILD_EXE%"=="" (
    echo [ERROR] MSBuild.exe not found!
    exit /b 1
)

echo [INFO] Using MSBuild: %MSBUILD_EXE%
"%MSBUILD_EXE%" SmartEventViewer.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild /m
