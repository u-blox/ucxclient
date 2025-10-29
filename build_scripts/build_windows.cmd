@echo off
REM Build script for ucxclient Windows GUI
REM This script builds the ucxclient library and prepares the GUI application
REM
REM Usage:
REM   build_windows.cmd          - Build Release and Debug configurations
REM   build_windows.cmd clean    - Clean intermediate files (keeps executables)
REM   build_windows.cmd cleanall - Clean ALL including executables and DLLs
REM   build_windows.cmd rebuild  - Clean all and rebuild from scratch

setlocal enabledelayedexpansion

echo ===================================
echo ucxclient Windows Build Script
echo ===================================
echo.

REM Change to project root directory (parent of build_scripts)
cd /d "%~dp0.."

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please run this script from the ucxclient root directory.
    pause
    exit /b 1
)

REM Parse command line argument
set BUILD_ACTION=build
if not "%1"=="" (
    set BUILD_ACTION=%1
)

REM Handle clean/cleanall/rebuild actions
if /i "%BUILD_ACTION%"=="clean" (
    echo Cleaning intermediate files...
    if exist "build" (
        cd build
        cmake --build . --target clean
        cd ..
        echo Clean complete!
    ) else (
        echo No build directory found.
    )
    goto :end
)

if /i "%BUILD_ACTION%"=="cleanall" (
    echo Cleaning ALL including executables and DLLs...
    if exist "build" (
        cd build
        cmake --build . --target cleanall
        cd ..
        echo Clean all complete!
    ) else (
        echo No build directory found.
    )
    goto :end
)

if /i "%BUILD_ACTION%"=="rebuild" (
    echo Performing full rebuild...
    if exist "build" (
        echo Removing build directory...
        rmdir /s /q build
    )
    echo Will create fresh build directory...
)

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found!
    echo Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

REM Create build directory
echo Creating build directory...
if not exist "build" mkdir build
cd build

REM Configure project
echo.
echo Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022"
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    echo.
    echo If you don't have Visual Studio 2022, try:
    echo   cmake .. -G "Visual Studio 16 2019"
    pause
    exit /b 1
)

REM Build Release configuration
echo.
echo Building Release configuration...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Release build failed!
    pause
    exit /b 1
)

REM Build Debug configuration (optional, don't fail if it doesn't work)
echo.
echo Building Debug configuration...
cmake --build . --config Debug
if errorlevel 1 (
    echo WARNING: Debug build failed, but Release succeeded.
)

REM Check if DLL was created
set DLL_FOUND=0
if exist "Release\ucxclient_windows.dll" (
    echo SUCCESS: Found Release\ucxclient_windows.dll
    set DLL_FOUND=1
)
if exist "Debug\ucxclient_windows.dll" (
    echo SUCCESS: Found Debug\ucxclient_windows.dll
    set DLL_FOUND=1
)

if %DLL_FOUND% == 0 (
    echo ERROR: ucxclient_windows.dll not found after build!
    echo Please check the build output for errors.
    pause
    exit /b 1
)

REM List build outputs
echo.
echo Build outputs:
if exist "Release" (
    echo Release:
    dir /b Release\*.exe Release\*.dll 2>nul
)
if exist "Debug" (
    echo Debug:
    dir /b Debug\*.exe Debug\*.dll 2>nul
)

REM Test the console application
echo.
echo Testing console application...
if exist "Release\windows_basic.exe" (
    echo Found Release\windows_basic.exe
    echo.
    echo Running port detection test...
    Release\windows_basic.exe --help
) else if exist "Debug\windows_basic.exe" (
    echo Found Debug\windows_basic.exe
    echo.
    echo Running port detection test...
    Debug\windows_basic.exe --help
) else (
    echo WARNING: windows_basic.exe not found
)

REM Return to root directory
cd ..

REM Check Python and GUI requirements
echo.
echo ===================================
echo Checking GUI Requirements
echo ===================================

python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    echo Please install Python 3.7 or higher.
    echo Download from: https://www.python.org/downloads/
    pause
    exit /b 1
)

echo Python found:
python --version

REM Test tkinter
python -c "import tkinter; print('tkinter available')" >nul 2>&1
if errorlevel 1 (
    echo ERROR: tkinter not available!
    echo Please install tkinter support for Python.
    pause
    exit /b 1
)

echo tkinter is available

REM Test the Python wrapper
echo.
echo Testing Python wrapper...
cd examples\python_gui
python -c "from ucx_wrapper import get_available_com_ports; ports = get_available_com_ports(); print(f'Found {len(ports)} COM ports: {ports}')"
if errorlevel 1 (
    echo WARNING: Python wrapper test failed
    echo The GUI may not work properly
) else (
    echo Python wrapper test successful
)
cd ..\.

cd ..

:end
echo.
echo ===================================
echo Build Complete!
echo ===================================
echo.
echo Usage:
echo   build_windows.cmd          - Build Release and Debug
echo   build_windows.cmd clean    - Clean intermediate files
echo   build_windows.cmd cleanall - Clean ALL including executables
echo   build_windows.cmd rebuild  - Full clean rebuild
echo.
echo To run the GUI application:
echo   launch_gui.cmd
echo.
echo To test with a COM port:
echo   build\Release\windows_basic.exe COM3
echo.

if /i not "%BUILD_ACTION%"=="clean" if /i not "%BUILD_ACTION%"=="cleanall" pause

endlocal
