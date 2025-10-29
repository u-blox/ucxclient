@echo off
REM Launch script for ucxclient Windows Console Application
REM This script builds the application, copies necessary DLLs, and runs it

setlocal enabledelayedexpansion

echo ===================================
echo ucxclient Windows App Launcher
echo ===================================
echo.

REM Change to project root directory
cd /d "%~dp0"

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please run this script from the ucxclient root directory.
    pause
    exit /b 1
)

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo Please install CMake and add it to your PATH.
    echo Download from: https://cmake.org/download/
    pause
    exit /b 1
)

REM Create build directory if needed
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Check if project is configured
if not exist "build\windows_app.vcxproj" (
    echo.
    echo Project not configured. Running CMake configuration...
    cd build
    
    REM Try Visual Studio 2022 first
    cmake .. -G "Visual Studio 17 2022" >nul 2>&1
    if errorlevel 1 (
        echo Visual Studio 2022 not found, trying 2019...
        cmake .. -G "Visual Studio 16 2019" >nul 2>&1
        if errorlevel 1 (
            echo Visual Studio 2019 not found, trying 2017...
            cmake .. -G "Visual Studio 15 2017" >nul 2>&1
            if errorlevel 1 (
                echo.
                echo ERROR: No compatible Visual Studio version found!
                echo Please install Visual Studio 2017 or later with C++ support.
                cd ..
                pause
                exit /b 1
            )
        )
    )
    
    cd ..
    echo CMake configuration complete.
)

REM Determine configuration to build (Debug by default, Release if requested)
set CONFIG=Debug
if /i "%1"=="release" set CONFIG=Release
if /i "%1"=="Release" set CONFIG=Release
if /i "%1"=="-r" set CONFIG=Release

REM Check if windows_app.exe is currently running
tasklist /FI "IMAGENAME eq windows_app.exe" 2>NUL | find /I /N "windows_app.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo.
    echo WARNING: windows_app.exe is currently running!
    echo Please close it before rebuilding.
    echo.
    choice /C YN /M "Do you want to forcefully kill it and continue"
    if errorlevel 2 (
        echo Build cancelled.
        pause
        exit /b 1
    )
    echo Killing windows_app.exe...
    taskkill /F /IM windows_app.exe >NUL 2>&1
    timeout /t 1 /nobreak >NUL
)

echo.
echo Building windows_app.exe (%CONFIG% configuration)...
echo.

REM Build the application
cmake --build build --config %CONFIG% --target windows_app
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!

REM Copy FTDI DLL to build directory if needed
set BUILD_DIR=build\%CONFIG%
set FTDI_DLL=examples\ftdi\ftd2xx64.dll

if not exist "%BUILD_DIR%\ftd2xx64.dll" (
    echo.
    echo Copying FTDI DLL to build directory...
    if exist "%FTDI_DLL%" (
        copy /Y "%FTDI_DLL%" "%BUILD_DIR%\" >nul
        echo FTDI DLL copied successfully.
    ) else (
        echo WARNING: %FTDI_DLL% not found!
        echo FTDI device detection may not work properly.
    )
) else (
    echo FTDI DLL already present in build directory.
)

REM Check if executable exists
if not exist "%BUILD_DIR%\windows_app.exe" (
    echo.
    echo ERROR: windows_app.exe not found in %BUILD_DIR%!
    pause
    exit /b 1
)

REM Launch the application
echo.
echo ===================================
echo Launching windows_app.exe...
echo ===================================
echo.

cd "%BUILD_DIR%"
windows_app.exe %2 %3 %4 %5 %6 %7 %8 %9

REM Store exit code
set APP_EXIT_CODE=%ERRORLEVEL%

REM Return to root directory
cd ..\..

REM Exit with application's exit code
exit /b %APP_EXIT_CODE%
