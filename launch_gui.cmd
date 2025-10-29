@echo off
REM Launch script for ucxclient Python GUI
REM This script launches the GUI application

echo ===================================
echo ucxclient Python GUI Launcher
echo ===================================
echo.

REM Check if we're in the right directory
if not exist "examples\python_gui\launcher.py" (
    echo ERROR: examples\python_gui\launcher.py not found!
    echo Please run this script from the ucxclient root directory.
    pause
    exit /b 1
)

REM Check for Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found!
    echo Please install Python 3.7 or higher.
    echo Download from: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM Check if DLL exists
if not exist "build\Release\ucxclient_windows.dll" (
    if not exist "build\Debug\ucxclient_windows.dll" (
        echo.
        echo WARNING: ucxclient_windows.dll not found!
        echo You need to build the library first.
        echo.
        echo Run: build_windows.cmd
        echo.
        pause
        exit /b 1
    )
)

REM Launch GUI
echo Launching GUI application...
echo.

REM Prevent Python from creating bytecode cache files
set PYTHONDONTWRITEBYTECODE=1

cd examples\python_gui
python -B launcher.py

REM Return to root directory
cd ..\..

