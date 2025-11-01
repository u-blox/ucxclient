@echo off
REM Build script for ucxclient Windows GUI
REM This script builds the ucxclient library and prepares the GUI application

echo ===================================
echo ucxclient Windows Build Script
echo ===================================
echo.

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
    pause
    exit /b 1
)

REM Create build directory
echo Creating build directory...
if not exist "build" mkdir build
cd build

REM Detect Visual Studio version
set VS_GENERATOR=""
for /f "tokens=*" %%i in ('cmake --help ^| findstr "Visual Studio"') do (
    echo Found: %%i
    REM Use the first Visual Studio generator found
    if !VS_GENERATOR! == "" (
        if "%%i" neq "" set VS_GENERATOR=%%i
    )
)

if "%VS_GENERATOR%" == "" (
    echo WARNING: No Visual Studio generator found, using default...
    set CMAKE_GENERATOR=
) else (
    echo Using generator: %VS_GENERATOR%
    set CMAKE_GENERATOR=-G "%VS_GENERATOR%"
)

REM Configure project
echo.
echo Configuring project with CMake...
cmake .. %CMAKE_GENERATOR%
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

REM Build Release configuration
echo.
echo Building Release configuration...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

REM Build Debug configuration
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
if exist "ucxclient_windows.dll" (
    echo SUCCESS: Found ucxclient_windows.dll
    set DLL_FOUND=1
)

if %DLL_FOUND% == 0 (
    echo ERROR: ucxclient_windows.dll not found after build!
    echo Please check the build output for errors.
    pause
    exit /b 1
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
cd ..\..

echo.
echo ===================================
echo Build Complete!
echo ===================================
echo.
echo To run the GUI application:
echo   launch_gui.cmd
echo.
echo To test with a COM port:
echo   build\Release\windows_basic.exe COM3 MyWi-FiSSID MyPassword
echo.

pause