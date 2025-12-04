@echo off
REM Launch script for ucx-windows-app

setlocal enabledelayedexpansion

echo ucxclient Windows App Launcher
echo.

REM Change to project root directory
cd /d "%~dp0"

REM Check for special commands (skip prerequisites for these)
if /i "%1"=="help" goto :help
if /i "%1"=="--help" goto :help
if /i "%1"=="-h" goto :help
if /i "%1"=="/?" goto :help
if /i "%1"=="sign" goto :sign
if /i "%1"=="signed" goto :launch_signed
if /i "%1"=="selftest" goto :selftest

REM Commands that need prerequisites
if /i "%1"=="clean" goto :clean_with_checks
if /i "%1"=="rebuild" goto :rebuild_with_checks
if /i "%1"=="all" goto :all_with_checks

REM Check prerequisites for normal launch
echo Checking prerequisites...
echo.

REM Check Windows version and architecture
for /f "tokens=4-5 delims=. " %%i in ('ver') do set WIN_VERSION=%%i.%%j
echo [INFO] Windows Version: %WIN_VERSION%

REM Check if 64-bit Windows
if not "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    if not "%PROCESSOR_ARCHITEW6432%"=="AMD64" (
        echo [ERROR] This application requires 64-bit Windows!
        echo.
        echo Your system is 32-bit: %PROCESSOR_ARCHITECTURE%
        echo Please use a 64-bit Windows installation.
        echo.
        exit /b 1
    )
)
echo [OK] 64-bit Windows detected
echo.

REM Check Windows 10/11 (version 10.0 or higher)
for /f "tokens=1,2 delims=." %%a in ("%WIN_VERSION%") do (
    set WIN_MAJOR=%%a
    set WIN_MINOR=%%b
)
if %WIN_MAJOR% LSS 10 (
    echo [WARNING] Windows 10 or 11 is recommended!
    echo [WARNING] Your version: Windows %WIN_VERSION%
    echo [WARNING] Build may not work on older Windows versions.
    echo.
)

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found!
    echo.
    echo CMake is required to build this project.
    echo Download from: https://cmake.org/download/
    echo Make sure to add CMake to system PATH during installation.
    echo.
    echo After installing CMake, restart PowerShell and try again.
    echo.
    exit /b 1
)
echo [OK] CMake found: 
cmake --version 2>&1 | findstr /C:"cmake version"
echo.

REM Check for Visual Studio (MSVC compiler)
where cl.exe >nul 2>&1
if errorlevel 1 (
    REM Try to find vswhere to locate VS installation
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo [ERROR] Visual Studio / MSVC compiler not found!
        echo.
        echo You need Visual Studio 2022 Build Tools or full Visual Studio 2022.
        echo.
        echo Option 1 - Build Tools only ^(minimal, ~2-3 GB^):
        echo   Download: https://aka.ms/vs/17/release/vs_BuildTools.exe
        echo   Select: Desktop development with C++
        echo.
        echo Option 2 - Full Visual Studio 2022 ^(complete IDE^):
        echo   Download: https://visualstudio.microsoft.com/downloads/
        echo   Install: Desktop development with C++ workload
        echo.
        echo After installation, restart PowerShell and try again.
        echo.
        exit /b 1
    )
    REM VS is installed but not in PATH - try to find it
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath -products *`) do (
        set "VSINSTALLDIR=%%i"
    )
    if not defined VSINSTALLDIR (
        echo [ERROR] Visual Studio found but C++ tools not installed!
        echo.
        echo Please run Visual Studio Installer and add:
        echo   Desktop development with C++
        echo.
        exit /b 1
    )
    echo [OK] Visual Studio installation detected
    echo      Location: !VSINSTALLDIR!
    echo      CMake will automatically find and use the compiler.
    echo.
) else (
    echo [OK] MSVC compiler found in PATH
    echo.
)

echo Prerequisites check passed!
echo.

REM Determine configuration
REM Default: Release (normal use)
REM With "debug" argument: Debug (development/troubleshooting)
set CONFIG=Release
set USER_SPECIFIED_CONFIG=0

REM Check if user specified debug
if /i "%1"=="debug" (
    set CONFIG=Debug
    set USER_SPECIFIED_CONFIG=1
    echo Using Debug configuration ^(user specified^)...
    echo.
) else if "%1"=="" (
    REM No argument = Release (default)
    set CONFIG=Release
    echo Using Release configuration ^(default^)...
    echo.
) else (
    REM First argument is not "debug" - assume it's a COM port or other parameter
    REM Use default Release configuration
    set CONFIG=Release
    echo Using Release configuration ^(default^)...
    echo.
)

REM Set build directory to ucx-windows-app/bin (where CMake outputs executables)
set BUILD_DIR=examples\ucx-windows-app\bin
set FTDI_DLL=examples\ucx-windows-app\third-party\ftdi\ftd2xx64.dll
set SETTINGS_FILE=ucx-windows-app.ini

REM Create settings file in project root if it doesn't exist
REM This allows the application to find and use a shared settings file
if not exist "%SETTINGS_FILE%" (
    echo.
    echo Creating settings file: %SETTINGS_FILE%
    echo # ucxclient Windows App Settings > "%SETTINGS_FILE%"
    echo # This file stores COM port and connection preferences >> "%SETTINGS_FILE%"
    echo. >> "%SETTINGS_FILE%"
)

REM Determine executable name based on configuration
if /i "!CONFIG!"=="Debug" (
    set EXE_NAME=ucx-windows-app-debug.exe
) else (
    set EXE_NAME=ucx-windows-app.exe
)

REM Check if executable exists and if source files are newer
set NEED_BUILD=0
set EXE_PATH=!BUILD_DIR!\!EXE_NAME!
set SOURCE_FILE=examples\ucx-windows-app\ucx-windows-app.c
set SIGNED_EXE=examples\ucx-windows-app\release\ucx-windows-app-signed.exe
set USE_SIGNED=0

if not exist "!EXE_PATH!" (
    REM Executable not found - check if signed version exists and source is untouched
    if exist "!SIGNED_EXE!" (
        echo.
        echo Executable not found in !BUILD_DIR!
        echo Checking for signed version and source modifications...
        
        REM Check if source has been modified by comparing with signed executable timestamp
        powershell -Command "if ((Get-Item '!SOURCE_FILE!').LastWriteTime -gt (Get-Item '!SIGNED_EXE!').LastWriteTime) { exit 1 }" >nul 2>&1
        if errorlevel 1 (
            echo [INFO] Source code has been modified - building from source...
            set NEED_BUILD=1
        ) else (
            echo [INFO] Source code is untouched - using signed version for fast start!
            echo.
            set USE_SIGNED=1
        )
    ) else (
        echo Executable not found. Auto-building !CONFIG!...
        set NEED_BUILD=1
    )
) else (
    echo.
    echo Checking build status...
    echo Executable: !EXE_PATH!
    for %%F in ("!EXE_PATH!") do echo   Build date: %%~tF
    echo Source file: !SOURCE_FILE!
    for %%F in ("!SOURCE_FILE!") do echo   Modified:   %%~tF
    
    REM Compare file timestamps (newer source file triggers rebuild)
    for %%S in ("!SOURCE_FILE!") do set SOURCE_TIME=%%~tS
    for %%E in ("!EXE_PATH!") do set EXE_TIME=%%~tE
    
    REM Simple comparison: if source is newer, rebuild
    if "!SOURCE_FILE!" NEQ "" (
        forfiles /P "examples\ucx-windows-app" /M "ucx-windows-app.c" /D +0 /C "cmd /c if @fdate GTR !EXE_TIME! set NEED_BUILD=1" 2>nul
        if exist "!SOURCE_FILE!" (
            powershell -Command "if ((Get-Item '!SOURCE_FILE!').LastWriteTime -gt (Get-Item '!EXE_PATH!').LastWriteTime) { exit 1 }" >nul 2>&1
            if errorlevel 1 (
                echo.
                echo [REBUILD NEEDED] Source file is newer than executable!
                set NEED_BUILD=1
            ) else (
                echo   Status: Executable is up-to-date
            )
        )
    )
)

if !NEED_BUILD! EQU 1 (
    echo.
    
    REM Configure if needed
    if not exist "build\CMakeCache.txt" (
        echo Configuring CMake...
        cmake -S examples\ucx-windows-app -B build
        if errorlevel 1 (
            echo ERROR: CMake configuration failed!
            exit /b 1
        )
    )
    
    REM Build
    echo Building !EXE_NAME! in !CONFIG! configuration...
    cmake --build build --config !CONFIG! --target ucx-windows-app
    if errorlevel 1 (
        echo ERROR: Build failed!
        exit /b 1
    )
    echo Build successful!
    echo.
    echo New executable built:
    for %%F in ("!EXE_PATH!") do echo   Build date: %%~tF
) else if !USE_SIGNED! EQU 0 (
    echo   Skipping build - executable is current
)

REM If using signed executable, skip to launch
if !USE_SIGNED! EQU 1 goto :launch_with_signed

REM Check if executable exists after build
if not exist "!EXE_PATH!" (
    echo ERROR: !EXE_NAME! not found in !BUILD_DIR!
    exit /b 1
)

REM For Debug configuration, just build and exit (don't launch)
if /i "%CONFIG%"=="Debug" (
    echo.
    echo ===================================
    echo Debug build complete!
    echo ===================================
    echo.
    echo Debug executable ready: !EXE_PATH!
    echo.
    echo To debug in VS Code:
    echo   1. Open workspace in VS Code
    echo   2. Set breakpoints in ucx-windows-app.c
    echo   3. Press F5 to start debugging (or Run and Debug ^> C/C++: ucx-windows-app^)
    echo.
    echo Or run directly: !EXE_PATH!
    echo.
    exit /b 0
)

REM Launch the application (Release only)
echo.

:launch_with_signed
REM If USE_SIGNED flag is set from earlier check, use signed version
if !USE_SIGNED! EQU 1 (
    set LAUNCH_EXE=!SIGNED_EXE!
    set LAUNCH_DIR=examples\ucx-windows-app\release
    echo Launching ucx-windows-app-signed.exe ^(for fast getting started^)...
    echo [INFO] This is the pre-built signed version.
    echo [INFO] If you modify the source code, it will be rebuilt automatically on next launch.
    goto :do_launch
)

REM Check for signed version in release folder
if exist "!SIGNED_EXE!" (
    REM Compare timestamps - use signed if it's newer than or equal to unsigned
    powershell -Command "if ((Get-Item '!SIGNED_EXE!').LastWriteTime -ge (Get-Item '!EXE_PATH!').LastWriteTime) { exit 0 } else { exit 1 }" >nul 2>&1
    if not errorlevel 1 (
        set LAUNCH_EXE=!SIGNED_EXE!
        set LAUNCH_DIR=examples\ucx-windows-app\release
        echo Launching ucx-windows-app-signed.exe ^(Signed Release - newer or equal^)...
    ) else (
        set LAUNCH_EXE=!EXE_PATH!
        set LAUNCH_DIR=!BUILD_DIR!
        echo Launching !EXE_NAME! ^(Unsigned - newer than signed version^)...
        echo [INFO] Signed version exists but is older. Use 'signed' to force signed version.
    )
) else (
    set LAUNCH_EXE=!EXE_PATH!
    set LAUNCH_DIR=!BUILD_DIR!
    echo Launching !EXE_NAME! ^(Unsigned Release^)...
)

:do_launch
echo.

cd "!LAUNCH_DIR!"

REM Get executable name from path
for %%F in ("!LAUNCH_EXE!") do set LAUNCH_EXE_NAME=%%~nxF

REM Pass arguments to Release build
!LAUNCH_EXE_NAME! %1 %2 %3 %4 %5 %6 %7 %8 %9

REM Store exit code
set APP_EXIT_CODE=%ERRORLEVEL%

REM Return to root
cd ..\..\..

REM Exit with application's exit code
exit /b %APP_EXIT_CODE%

REM ===================================
REM Launch Signed command
REM ===================================
:launch_signed
echo ===================================
echo Launching Signed Release
echo ===================================
echo.

set SIGNED_EXE=examples\ucx-windows-app\release\ucx-windows-app-signed.exe

if not exist "!SIGNED_EXE!" (
    echo [ERROR] Signed executable not found: !SIGNED_EXE!
    echo.
    echo Please sign the executable first:
    echo   launch-ucx-windows-app.cmd sign [thumbprint]
    echo.
    exit /b 1
)

echo Launching ucx-windows-app-signed.exe...
echo.

cd examples\ucx-windows-app\release

REM Pass arguments (skip first argument which is --signed)
ucx-windows-app-signed.exe %2 %3 %4 %5 %6 %7 %8 %9

REM Store exit code
set APP_EXIT_CODE=%ERRORLEVEL%

REM Return to root
cd ..\..\..

REM Exit with application's exit code
exit /b %APP_EXIT_CODE%

REM ===================================
REM Clean command (with checks)
REM ===================================
:clean_with_checks
REM Just continue to clean, prerequisites already checked
:clean
echo ===================================
echo Deep Cleaning Build Directory
echo ===================================
echo.

REM Determine which config to clean
set CLEAN_CONFIG=%2
if "%CLEAN_CONFIG%"=="" (
    echo Cleaning ALL configurations...
    echo.
    
    REM CMake clean for Debug
    if exist "build\Debug" (
        echo [1/4] CMake clean Debug...
        cmake --build build --config Debug --target clean 2>nul
    )
    
    REM CMake clean for Release
    if exist "build\Release" (
        echo [2/4] CMake clean Release...
        cmake --build build --config Release --target clean 2>nul
    )
    
    REM Deep clean - remove all object files
    echo [3/4] Removing all .obj files...
    if exist "build\*.dir" (
        for /d %%d in (build\*.dir) do (
            if exist "%%d\Debug" rd /s /q "%%d\Debug" 2>nul
            if exist "%%d\Release" rd /s /q "%%d\Release" 2>nul
        )
    )
    
    REM Remove intermediate files
    echo [4/4] Removing .pdb and .ilk files...
    del /s /q build\*.ilk 2>nul
    if exist "build\Debug" del /q build\Debug\*.pdb 2>nul
    if exist "build\Release" del /q build\Release\*.pdb 2>nul
    
) else (
    if /i "%CLEAN_CONFIG%"=="debug" set CLEAN_CONFIG=Debug
    if /i "%CLEAN_CONFIG%"=="release" set CLEAN_CONFIG=Release
    echo Cleaning %CLEAN_CONFIG% configuration...
    echo.
    
    REM CMake clean
    echo [1/3] CMake clean %CLEAN_CONFIG%...
    cmake --build build --config %CLEAN_CONFIG% --target clean 2>nul
    
    REM Deep clean - remove object files for this config
    echo [2/3] Removing all .obj files for %CLEAN_CONFIG%...
    if exist "build\*.dir" (
        for /d %%d in (build\*.dir) do (
            if exist "%%d\%CLEAN_CONFIG%" rd /s /q "%%d\%CLEAN_CONFIG%" 2>nul
        )
    )
    
    REM Remove intermediate files
    echo [3/3] Removing .pdb and .ilk files for %CLEAN_CONFIG%...
    del /s /q build\*.ilk 2>nul
    if exist "build\%CLEAN_CONFIG%" del /q build\%CLEAN_CONFIG%\*.pdb 2>nul
)

echo.
echo ===================================
echo Clean complete!
echo ===================================
echo All object files, executables, and intermediate files removed.
echo.
exit /b 0

REM ===================================
REM Rebuild command (with checks)
REM ===================================
:rebuild_with_checks
REM Just continue to rebuild, prerequisites already checked
:rebuild
echo Rebuild requested...
echo.

REM Determine configuration (default to Release)
set REBUILD_CONFIG=Release
if /i "%2"=="debug" set REBUILD_CONFIG=Debug
if /i "%2"=="Debug" set REBUILD_CONFIG=Debug
if /i "%2"=="-d" set REBUILD_CONFIG=Debug

echo Rebuilding %REBUILD_CONFIG% configuration...
echo.

REM Clean first
echo Step 1: Cleaning...
cmake --build build --config %REBUILD_CONFIG% --target clean 2>nul

REM Force CMake reconfigure to pick up latest Git commit count
echo Step 2: Reconfiguring CMake...
del /Q build\CMakeCache.txt 2>nul
cmake -S examples\ucx-windows-app -B build
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

REM Build
echo Step 3: Building...
cmake --build build --config %REBUILD_CONFIG% --target ucx-windows-app
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ===================================
echo Rebuild complete!
echo ===================================
exit /b 0

REM ===================================
REM Build All command (with checks)
REM ===================================
:all_with_checks
REM Just continue to all, prerequisites already checked
:all
echo ===================================
echo Building ALL Configurations
echo ===================================
echo.
echo This will build both Debug and Release configurations.
echo.

REM Configure if needed
if not exist "build\ucx-windows-app.vcxproj" (
    echo Configuring CMake...
    cmake -S examples\ucx-windows-app -B build
    if errorlevel 1 (
        echo ERROR: CMake configuration failed!
        exit /b 1
    )
    echo.
)

REM Build Debug
echo ===================================
echo [1/2] Building Debug Configuration
echo ===================================
echo.
cmake --build build --config Debug --target ucx-windows-app
if errorlevel 1 (
    echo ERROR: Debug build failed!
    exit /b 1
)

echo Debug build complete!
echo.

REM Build Release
echo ===================================
echo [2/2] Building Release Configuration
echo ===================================
echo.
cmake --build build --config Release --target ucx-windows-app
if errorlevel 1 (
    echo ERROR: Release build failed!
    exit /b 1
)

echo Release build complete!
echo.

echo ===================================
echo All Configurations Built Successfully!
echo ===================================
echo.
echo Output files:
echo   examples\ucx-windows-app\bin\ucx-windows-app-debug.exe
echo   examples\ucx-windows-app\bin\ucx-windows-app.exe
echo.
exit /b 0

REM ===================================
REM Sign command
REM ===================================
:sign
echo ===================================
echo Code Signing ucx-windows-app.exe
echo ===================================
echo.

REM Always sign Release configuration
set SIGN_CONFIG=Release

set SIGN_EXE=examples\ucx-windows-app\bin\ucx-windows-app.exe
set SIGN_EXE_SIGNED=examples\ucx-windows-app\release\ucx-windows-app-signed.exe
set CERT_THUMBPRINT=%2

REM Check if executable exists
if not exist "!SIGN_EXE!" (
    echo [ERROR] Executable not found: !SIGN_EXE!
    echo.
    echo Build the Release version first:
    echo   launch-ucx-windows-app.cmd rebuild
    echo.
    exit /b 1
)

REM Check if thumbprint provided
if not defined CERT_THUMBPRINT (
    echo [ERROR] Certificate thumbprint required!
    echo.
    echo USAGE:
    echo   launch-ucx-windows-app.cmd sign [thumbprint]
    echo.
    echo EXAMPLE:
    echo   launch-ucx-windows-app.cmd sign 1234567890ABCDEF...
    echo.
    echo To find your certificate thumbprint:
    echo   1. Open Certificate Manager: certmgr.msc
    echo   2. Navigate to: Personal ^> Certificates
    echo   3. Double-click your code signing certificate
    echo   4. Go to Details tab
    echo   5. Scroll down to Thumbprint
    echo   6. Copy the value (remove spaces^)
    echo.
    exit /b 1
)

echo Configuration: !SIGN_CONFIG!
echo Executable:    !SIGN_EXE!
echo Thumbprint:    !CERT_THUMBPRINT!
echo.

REM Find SignTool.exe
set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
if not exist "!SIGNTOOL!" (
    REM Try alternative location
    set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\x64\signtool.exe"
)
if not exist "!SIGNTOOL!" (
    echo [ERROR] SignTool.exe not found!
    echo.
    echo Please install Windows SDK 10:
    echo   https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
    echo.
    exit /b 1
)

echo [INFO] Using SignTool: !SIGNTOOL!
echo.
echo Signing executable...
echo.

REM Sign with certificate
"!SIGNTOOL!" sign /sha1 !CERT_THUMBPRINT! /tr http://timestamp.digicert.com /td sha256 /fd sha256 "!SIGN_EXE!"

if errorlevel 1 (
    echo.
    echo [ERROR] Signing failed!
    echo.
    echo Troubleshooting:
    echo   1. Make sure USB certificate token is inserted
    echo   2. Check if SafeNet Authentication Client is running
    echo   3. Verify certificate thumbprint is correct
    echo   4. Try: certmgr.msc to verify certificate is available
    echo.
    exit /b 1
)

echo.
echo ===================================
echo Verifying signature...
echo ===================================
echo.

"!SIGNTOOL!" verify /pa /v "!SIGN_EXE!"

if errorlevel 1 (
    echo.
    echo [WARNING] Signature verification had issues
    echo.
    exit /b 1
)

echo.
echo ===================================
echo Copying to release folder...
echo ===================================
echo.

REM Create release folder if it doesn't exist
if not exist "examples\ucx-windows-app\release" (
    mkdir "examples\ucx-windows-app\release"
)

REM Copy the signed executable to release folder
copy /Y "!SIGN_EXE!" "!SIGN_EXE_SIGNED!"
if errorlevel 1 (
    echo [ERROR] Failed to copy signed executable!
    exit /b 1
)

echo Copied: !SIGN_EXE!
echo     To: !SIGN_EXE_SIGNED!
echo.

echo ===================================
echo Signing completed successfully!
echo ===================================
echo Signed executable: !SIGN_EXE_SIGNED!
echo Original unsigned: !SIGN_EXE!
echo.
echo The release folder contains the signed executable ready for distribution.
echo.

exit /b 0

REM ===================================
REM Help command
REM ===================================
:help
echo.
echo USAGE:
echo   launch-ucx-windows-app.cmd [options] [arguments]
echo.
echo BASIC USAGE:
echo   (no args)             Run Release build (auto-selects signed if newer)
echo   debug                 Run Debug build (builds if needed)
echo   signed                Force run signed version from release folder
echo.
echo   By default, launcher checks both bin/ and release/ folders and runs
echo   the newest version. Use --signed to force the signed version.
echo.
echo   All other arguments are passed to the application.
echo.
echo COMMANDS:
echo   clean [config]        Deep clean build artifacts
echo                         Removes ALL .obj, .exe, .pdb, .ilk files
echo                         - No config = clean all (Debug + Release)
echo                         - 'debug' or 'release' = clean specific config
echo.
echo   rebuild [config]      Deep clean and rebuild from scratch
echo                         Forces recompilation of ALL source files
echo                         - No config = rebuild Release
echo                         - 'debug' = rebuild Debug
echo                         - 'release' = rebuild Release
echo.
echo   all                   Build both Debug and Release configurations
echo                         Builds all configurations from current state
echo                         (use 'clean' first for a fresh build)
echo.
echo   sign [thumbprint]     Code sign the Release executable
echo                         - thumbprint: Certificate thumbprint (required)
echo                         Example: sign 1234567890ABCDEF...
echo.
echo   selftest              Run comprehensive workspace tests
echo                         Tests workspace structure, builds, version checks
echo.
echo   help / --help / -h    Show this help message
echo.
echo EXAMPLES:
echo   launch-ucx-windows-app.cmd
echo       Launch Release build (auto-selects signed if newer)
echo.
echo   launch-ucx-windows-app.cmd signed
echo       Force launch signed version from release folder
echo.
echo   launch-ucx-windows-app.cmd debug
echo       Launch Debug build (auto-builds if needed)
echo.
echo   launch-ucx-windows-app.cmd COM4
echo       Launch Release build and pass COM4 to the app
echo.
echo   launch-ucx-windows-app.cmd signed COM4
echo       Launch signed version and pass COM4 to the app
echo.
echo   launch-ucx-windows-app.cmd debug COM4
echo       Launch Debug build and pass COM4 to the app
echo.
echo   launch-ucx-windows-app.cmd clean
echo       Clean all configurations (Debug and Release)
echo.
echo   launch-ucx-windows-app.cmd clean debug
echo       Clean only Debug configuration
echo.
echo   launch-ucx-windows-app.cmd rebuild
echo       Clean and rebuild Release configuration
echo.
echo   launch-ucx-windows-app.cmd rebuild debug
echo       Clean and rebuild Debug configuration
echo.
echo   launch-ucx-windows-app.cmd all
echo       Build both Debug and Release configurations
echo.
echo   launch-ucx-windows-app.cmd sign 1234567890ABCDEF...
echo       Sign Release build and save to ucx-windows-app-signed.exe
echo.
echo   launch-ucx-windows-app.cmd selftest
echo       Run comprehensive workspace tests and validation
echo.
echo NOTES:
echo   - Release is the default (optimized, no debug symbols)
echo   - Debug is for development/troubleshooting (symbols, no optimization)
echo   - First launch auto-configures CMake if needed
echo   - Auto-builds if executable is missing or outdated
echo   - Settings file: ucx-windows-app.ini in project root (launcher creates it)
echo   - Exe-only usage: Creates ucx-windows-app.ini next to executable
echo.
exit /b 0

REM ===================================
REM Selftest command
REM ===================================
:selftest
echo.
echo ========================================
echo ucx-windows-app SELFTEST
echo ========================================
echo.
echo This will verify the workspace integrity by checking:
echo   - Directory structure
echo   - Required source files
echo   - Build artifacts
echo   - Executable version information
echo   - CMake configuration
echo.

set "SELFTEST_ERRORS=0"
set "SELFTEST_WARNINGS=0"

REM Check current directory is project root
if not exist "examples\ucx-windows-app\ucx-windows-app.c" (
    echo [ERROR] Not in ucxclient project root!
    echo Current directory: %CD%
    echo Expected to find: examples\ucx-windows-app\ucx-windows-app.c
    exit /b 1
)
echo [OK] Working directory is ucxclient project root

REM Check critical directory structure
echo.
echo Checking directory structure...
set "REQUIRED_DIRS=src inc ucx_api examples\ucx-windows-app build"
for %%d in (%REQUIRED_DIRS%) do (
    if exist "%%d\" (
        echo [OK] %%d\
    ) else (
        echo [ERROR] Missing directory: %%d\
        set /a SELFTEST_ERRORS+=1
    )
)

REM Check critical source files
echo.
echo Checking critical source files...
set "REQUIRED_FILES=src\u_cx_at_client.c inc\u_cx_at_client.h examples\ucx-windows-app\ucx-windows-app.c examples\ucx-windows-app\CMakeLists.txt examples\ucx-windows-app\README.md"
for %%f in (%REQUIRED_FILES%) do (
    if exist "%%f" (
        echo [OK] %%f
    ) else (
        echo [ERROR] Missing file: %%f
        set /a SELFTEST_ERRORS+=1
    )
)

REM Check ucx_api generated files
echo.
echo Checking UCX API generated files...
if exist "ucx_api\generated\" (
    set "API_FILES=0"
    for %%f in (ucx_api\generated\u_cx_*.h) do set /a API_FILES+=1
    if !API_FILES! GTR 10 (
        echo [OK] ucx_api\generated\ contains !API_FILES! header files
    ) else (
        echo [WARNING] ucx_api\generated\ has only !API_FILES! files ^(expected more^)
        set /a SELFTEST_WARNINGS+=1
    )
) else (
    echo [ERROR] Missing ucx_api\generated\ directory
    set /a SELFTEST_ERRORS+=1
)

REM Check build directory and CMake configuration
echo.
echo Checking build configuration...
if exist "build\CMakeCache.txt" (
    echo [OK] CMake is configured ^(build\CMakeCache.txt exists^)
) else (
    echo [WARNING] CMake not configured yet ^(build\CMakeCache.txt missing^)
    echo          Run the launcher without arguments to auto-configure
    set /a SELFTEST_WARNINGS+=1
)

if exist "build\ucx-windows-app.sln" (
    echo [OK] Visual Studio solution generated
) else (
    echo [WARNING] ucx-windows-app.sln not found
    echo          Run the launcher to generate build files
    set /a SELFTEST_WARNINGS+=1
)

REM Check executables
echo.
echo Checking executables...
set "RELEASE_EXE=examples\ucx-windows-app\bin\ucx-windows-app.exe"
set "DEBUG_EXE=examples\ucx-windows-app\bin\ucx-windows-app-debug.exe"
set "SIGNED_EXE=examples\ucx-windows-app\release\ucx-windows-app-signed.exe"

if exist "!RELEASE_EXE!" (
    echo [OK] Release executable exists
    REM Check version
    for /f "delims=" %%v in ('"!RELEASE_EXE!" -v 2^>nul') do set "RELEASE_VER=%%v"
    if defined RELEASE_VER (
        echo [OK] Release executable can run - Version: !RELEASE_VER!
    ) else (
        echo [WARNING] Release executable exists but -v flag failed
        set /a SELFTEST_WARNINGS+=1
    )
) else (
    echo [INFO] Release executable not built yet
)

if exist "!DEBUG_EXE!" (
    echo [OK] Debug executable exists
    REM Check version
    for /f "delims=" %%v in ('"!DEBUG_EXE!" -v 2^>nul') do set "DEBUG_VER=%%v"
    if defined DEBUG_VER (
        echo [OK] Debug executable can run - Version: !DEBUG_VER!
    ) else (
        echo [WARNING] Debug executable exists but -v flag failed
        set /a SELFTEST_WARNINGS+=1
    )
) else (
    echo [INFO] Debug executable not built yet ^(use: launch-ucx-windows-app.cmd debug^)
)

if exist "!SIGNED_EXE!" (
    echo [OK] Signed executable exists
    REM Check version
    for /f "delims=" %%v in ('"!SIGNED_EXE!" -v 2^>nul') do set "SIGNED_VER=%%v"
    if defined SIGNED_VER (
        echo [OK] Signed executable can run - Version: !SIGNED_VER!
    ) else (
        echo [WARNING] Signed executable exists but -v flag failed
        set /a SELFTEST_WARNINGS+=1
    )
) else (
    echo [INFO] Signed executable not created yet ^(use: launch-ucx-windows-app.cmd sign^)
)

REM Check launcher script itself
echo.
echo Checking launcher script...
if exist "launch-ucx-windows-app.cmd" (
    echo [OK] Launcher script exists
) else (
    echo [ERROR] Launcher script not found!
    set /a SELFTEST_ERRORS+=1
)

REM Check for Git repository
echo.
echo Checking Git repository...
git rev-parse --git-dir >nul 2>&1
if !errorlevel! equ 0 (
    echo [OK] Git repository detected
    for /f "tokens=*" %%b in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set "GIT_BRANCH=%%b"
    echo     Current branch: !GIT_BRANCH!
    for /f "tokens=*" %%c in ('git rev-list --count HEAD 2^>nul') do set "COMMIT_COUNT=%%c"
    echo     Commit count: !COMMIT_COUNT!
    
    REM Get remote URL
    for /f "tokens=*" %%r in ('git config --get remote.origin.url 2^>nul') do set "GIT_REMOTE=%%r"
    if defined GIT_REMOTE (
        echo     Remote: !GIT_REMOTE!
    )
) else (
    echo [WARNING] Not a Git repository or Git not installed
    set /a SELFTEST_WARNINGS+=1
)

REM Ask about extended tests (master branch comparison)
echo.
echo ========================================
echo EXTENDED TESTS (Optional)
echo ========================================
echo.
echo Do you want to clone master branch for comparison?
echo This will:
echo   - Clone master branch to temp folder
echo   - Compare directory structure
echo   - Show differences in file counts
echo   - Report any missing critical files
echo.
set /p CLONE_MASTER="Clone master branch? (y/N): "

if /i "!CLONE_MASTER!"=="y" (
    echo.
    echo Starting master branch clone and comparison...
    call :clone_and_compare_master
) else (
    echo.
    echo Skipping master branch comparison.
)

REM Summary
echo.
echo ========================================
echo SELFTEST SUMMARY
echo ========================================
if !SELFTEST_ERRORS! equ 0 (
    if !SELFTEST_WARNINGS! equ 0 (
        echo [SUCCESS] All tests passed!
        echo.
        echo Your workspace is ready for development and release.
    ) else (
        echo [PARTIAL] Tests passed with !SELFTEST_WARNINGS! warning^(s^)
        echo.
        echo Warnings are typically about missing optional builds.
        echo Run the launcher to build and resolve warnings.
    )
) else (
    echo [FAILED] Found !SELFTEST_ERRORS! error^(s^) and !SELFTEST_WARNINGS! warning^(s^)
    echo.
    echo Please fix the errors above before proceeding.
)
echo ========================================
echo.

exit /b !SELFTEST_ERRORS!

REM ===================================
REM Clone and compare master branch
REM ===================================
:clone_and_compare_master
echo.
echo =======================================
echo MASTER BRANCH COMPARISON
echo =======================================
echo.

REM Create temp directory
set "TEMP_DIR=%TEMP%\ucxclient-master-%RANDOM%"
echo Creating temp directory: !TEMP_DIR!
mkdir "!TEMP_DIR!" 2>nul

REM Get remote URL
for /f "tokens=*" %%r in ('git config --get remote.origin.url 2^>nul') do set "REPO_URL=%%r"
if not defined REPO_URL (
    echo [ERROR] Cannot determine Git remote URL
    echo Make sure you're in a Git repository with a remote configured.
    goto :cleanup_temp
)

echo Cloning master branch from: !REPO_URL!
echo Please wait, this may take a minute...
echo.

REM Clone only master branch (shallow clone for speed)
git clone --depth 1 --branch master --single-branch "!REPO_URL!" "!TEMP_DIR!"
if !errorlevel! neq 0 (
    echo.
    echo [ERROR] Failed to clone master branch
    echo This could be because:
    echo   - No network connection
    echo   - Master branch doesn't exist
    echo   - Authentication required
    goto :cleanup_temp
)

echo [OK] Master branch cloned successfully
echo.

REM Compare directory structure
echo Comparing directory structure...
echo.

set "COMPARE_DIRS=src inc ucx_api examples build"
for %%d in (!COMPARE_DIRS!) do (
    if exist "%%d\" (
        if exist "!TEMP_DIR!\%%d\" (
            echo [MATCH] %%d\ - exists in both branches
        ) else (
            echo [LOCAL ONLY] %%d\ - exists locally but NOT in master
        )
    ) else (
        if exist "!TEMP_DIR!\%%d\" (
            echo [MASTER ONLY] %%d\ - exists in master but NOT locally
        )
    )
)

echo.
echo Checking if examples\ucx-windows-app exists in master...
if exist "!TEMP_DIR!\examples\ucx-windows-app\" (
    echo [INFO] examples\ucx-windows-app\ EXISTS in master
    echo.
    echo Comparing file counts:
    
    REM Count .c files
    set "LOCAL_C=0"
    set "MASTER_C=0"
    for %%f in (examples\ucx-windows-app\*.c) do set /a LOCAL_C+=1
    for %%f in (!TEMP_DIR!\examples\ucx-windows-app\*.c) do set /a MASTER_C+=1
    echo   Local .c files:  !LOCAL_C!
    echo   Master .c files: !MASTER_C!
    
    REM Count .h files
    set "LOCAL_H=0"
    set "MASTER_H=0"
    for %%f in (examples\ucx-windows-app\*.h) do set /a LOCAL_H+=1
    for %%f in (!TEMP_DIR!\examples\ucx-windows-app\*.h) do set /a MASTER_H+=1
    echo   Local .h files:  !LOCAL_H!
    echo   Master .h files: !MASTER_H!
) else (
    echo [INFO] examples\ucx-windows-app\ does NOT exist in master
    echo       This is your new contribution not yet merged!
)

echo.
echo Comparing core library files...
set "CORE_FILES=src\u_cx_at_client.c inc\u_cx_at_client.h"
for %%f in (!CORE_FILES!) do (
    if exist "%%f" (
        if exist "!TEMP_DIR!\%%f" (
            fc /b "%%f" "!TEMP_DIR!\%%f" >nul 2>&1
            if !errorlevel! equ 0 (
                echo [IDENTICAL] %%f
            ) else (
                echo [MODIFIED] %%f - differs from master
            )
        ) else (
            echo [LOCAL ONLY] %%f - not in master
        )
    )
)

echo.
echo Checking ucx_api header count...
set "LOCAL_API=0"
set "MASTER_API=0"
for %%f in (ucx_api\generated\u_cx_*.h) do set /a LOCAL_API+=1
if exist "!TEMP_DIR!\ucx_api\generated\" (
    for %%f in (!TEMP_DIR!\ucx_api\generated\u_cx_*.h) do set /a MASTER_API+=1
)
echo   Local UCX API headers:  !LOCAL_API!
echo   Master UCX API headers: !MASTER_API!

:cleanup_temp
echo.
echo =======================================
echo.
set /p REMOVE_TEMP="Remove temp folder (!TEMP_DIR!)? (Y/n): "
if /i "!REMOVE_TEMP!"=="n" (
    echo.
    echo Temp folder kept at: !TEMP_DIR!
    echo Remember to delete it later.
) else (
    echo.
    echo Removing temp folder...
    rmdir /s /q "!TEMP_DIR!" 2>nul
    if exist "!TEMP_DIR!" (
        echo [WARNING] Could not remove temp folder: !TEMP_DIR!
        echo Please delete it manually.
    ) else (
        echo [OK] Temp folder removed
    )
)

echo.
goto :eof
