@echo off
REM Launch script for ucxclient-x64

setlocal enabledelayedexpansion

echo ===================================
echo ucxclient-x64 Launcher
echo ===================================
echo.

REM Change to project root directory
cd /d "%~dp0"

REM Check for special commands (skip prerequisites for these)
if /i "%1"=="help" goto :help
if /i "%1"=="--help" goto :help
if /i "%1"=="-h" goto :help
if /i "%1"=="/?" goto :help
if /i "%1"=="sign" goto :sign

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
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
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

REM Set build directory
set BUILD_DIR=build\!CONFIG!
set FTDI_DLL=examples\ftdi\ftd2xx64.dll

REM Check if executable exists and if source files are newer
set NEED_BUILD=0
set EXE_PATH=!BUILD_DIR!\ucxclient-x64.exe
set SOURCE_FILE=examples\ucxclient-x64.c

if not exist "!EXE_PATH!" (
    echo Executable not found. Auto-building !CONFIG!...
    set NEED_BUILD=1
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
        forfiles /P "examples" /M "ucxclient-x64.c" /D +0 /C "cmd /c if @fdate GTR !EXE_TIME! set NEED_BUILD=1" 2>nul
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
    if not exist "build\ucxclient-x64.vcxproj" (
        echo Configuring CMake...
        cmake -S . -B build
        if errorlevel 1 (
            echo ERROR: CMake configuration failed!
            exit /b 1
        )
    )
    
    REM Build
    echo Building ucxclient-x64.exe in !CONFIG! configuration...
    cmake --build build --config !CONFIG! --target ucxclient-x64
    if errorlevel 1 (
        echo ERROR: Build failed!
        exit /b 1
    )
    echo Build successful!
    echo.
    echo New executable built:
    for %%F in ("!EXE_PATH!") do echo   Build date: %%~tF
) else (
    echo   Skipping build - executable is current
)

REM Copy FTDI DLL if needed
if not exist "!BUILD_DIR!\ftd2xx64.dll" (
    if exist "!FTDI_DLL!" (
        echo Copying FTDI DLL...
        copy /Y "!FTDI_DLL!" "!BUILD_DIR!\" >nul
    )
)

REM Check if executable exists after build
if not exist "!BUILD_DIR!\ucxclient-x64.exe" (
    echo ERROR: ucxclient-x64.exe not found in !BUILD_DIR!
    exit /b 1
)

REM Launch the application
echo.
echo ===================================
echo Launching ucxclient-x64.exe...
echo ===================================
echo.

cd "!BUILD_DIR!"

REM Pass arguments correctly based on whether "debug" was first argument
if /i "%1"=="debug" (
    REM First arg was "debug", so pass %2 onwards
    ucxclient-x64.exe %2 %3 %4 %5 %6 %7 %8 %9
) else (
    REM No "debug" arg, so pass %1 onwards
    ucxclient-x64.exe %1 %2 %3 %4 %5 %6 %7 %8 %9
)

REM Store exit code
set APP_EXIT_CODE=%ERRORLEVEL%

REM Return to root
cd ..\..

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

REM Determine configuration
set REBUILD_CONFIG=Debug
if /i "%2"=="release" set REBUILD_CONFIG=Release
if /i "%2"=="Release" set REBUILD_CONFIG=Release
if /i "%2"=="-r" set REBUILD_CONFIG=Release

echo Rebuilding %REBUILD_CONFIG% configuration...
echo.

REM Clean first
echo Step 1: Cleaning...
cmake --build build --config %REBUILD_CONFIG% --target clean 2>nul

REM Configure if needed
if not exist "build\ucxclient-x64.vcxproj" (
    echo Step 2: Configuring CMake...
    cmake -S . -B build
    if errorlevel 1 (
        echo ERROR: CMake configuration failed!
        pause
        exit /b 1
    )
)

REM Build
echo Step 3: Building...
cmake --build build --config %REBUILD_CONFIG% --target ucxclient-x64
if errorlevel 1 (
    echo ERROR: Build failed!
    exit /b 1
)

REM Copy FTDI DLL
set BUILD_DIR=build\!REBUILD_CONFIG!
set FTDI_DLL=examples\ftdi\ftd2xx64.dll
if exist "!FTDI_DLL!" (
    echo Step 4: Copying FTDI DLL...
    copy /Y "!FTDI_DLL!" "!BUILD_DIR!\" >nul
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
if not exist "build\ucxclient-x64.vcxproj" (
    echo Configuring CMake...
    cmake -S . -B build
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
cmake --build build --config Debug --target ucxclient-x64
if errorlevel 1 (
    echo ERROR: Debug build failed!
    exit /b 1
)

REM Copy FTDI DLL for Debug
set FTDI_DLL=examples\ftdi\ftd2xx64.dll
if exist "%FTDI_DLL%" (
    echo Copying FTDI DLL to Debug...
    copy /Y "%FTDI_DLL%" "build\Debug\" >nul
)
echo Debug build complete!
echo.

REM Build Release
echo ===================================
echo [2/2] Building Release Configuration
echo ===================================
echo.
cmake --build build --config Release --target ucxclient-x64
if errorlevel 1 (
    echo ERROR: Release build failed!
    exit /b 1
)

REM Copy FTDI DLL for Release
if exist "%FTDI_DLL%" (
    echo Copying FTDI DLL to Release...
    copy /Y "%FTDI_DLL%" "build\Release\" >nul
)
echo Release build complete!
echo.

echo ===================================
echo All Configurations Built Successfully!
echo ===================================
echo.
echo Output files:
echo   build\Debug\ucxclient-x64.exe
echo   build\Release\ucxclient-x64.exe
echo.
exit /b 0

REM ===================================
REM Sign command
REM ===================================
:sign
echo ===================================
echo Code Signing ucxclient-x64.exe
echo ===================================
echo.

REM Always sign Release configuration
set SIGN_CONFIG=Release

set SIGN_EXE=build\!SIGN_CONFIG!\ucxclient-x64.exe
set SIGN_EXE_SIGNED=build\Release_Signed\ucxclient-x64-signed.exe
set CERT_THUMBPRINT=%2

REM Check if executable exists
if not exist "!SIGN_EXE!" (
    echo [ERROR] Executable not found: !SIGN_EXE!
    echo.
    echo Build the Release version first:
    echo   launch_ucxclient-x64.cmd rebuild
    echo.
    exit /b 1
)

REM Check if thumbprint provided
if not defined CERT_THUMBPRINT (
    echo [ERROR] Certificate thumbprint required!
    echo.
    echo USAGE:
    echo   launch_ucxclient-x64.cmd sign [thumbprint]
    echo.
    echo EXAMPLE:
    echo   launch_ucxclient-x64.cmd sign 1234567890ABCDEF...
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
echo Copying to Release_Signed folder...
echo ===================================
echo.

REM Create Release_Signed folder if it doesn't exist
if not exist "build\Release_Signed" (
    mkdir "build\Release_Signed"
)

REM Copy the signed executable to Release_Signed folder
copy /Y "!SIGN_EXE!" "!SIGN_EXE_SIGNED!"
if errorlevel 1 (
    echo [ERROR] Failed to copy signed executable!
    exit /b 1
)

REM Also copy FTDI DLL to Release_Signed folder
if exist "%FTDI_DLL%" (
    copy /Y "%FTDI_DLL%" "build\Release_Signed\ftd2xx64.dll" >nul 2>&1
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

exit /b 0

REM ===================================
REM Help command
REM ===================================
:help
echo.
echo USAGE:
echo   launch_ucxclient-x64.cmd [debug] [arguments]
echo.
echo BASIC USAGE:
echo   (no args)             Run Release build (builds if needed)
echo   debug                 Run Debug build (builds if needed)
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
echo   help / --help / -h    Show this help message
echo.
echo EXAMPLES:
echo   launch_ucxclient-x64.cmd
echo       Launch Release build (auto-builds if needed)
echo.
echo   launch_ucxclient-x64.cmd debug
echo       Launch Debug build (auto-builds if needed)
echo.
echo   launch_ucxclient-x64.cmd COM4
echo       Launch Release build and pass COM4 to the app
echo.
echo   launch_ucxclient-x64.cmd debug COM4
echo       Launch Debug build and pass COM4 to the app
echo.
echo   launch_ucxclient-x64.cmd clean
echo       Clean all configurations (Debug and Release)
echo.
echo   launch_ucxclient-x64.cmd clean debug
echo       Clean only Debug configuration
echo.
echo   launch_ucxclient-x64.cmd rebuild
echo       Clean and rebuild Release configuration
echo.
echo   launch_ucxclient-x64.cmd rebuild debug
echo       Clean and rebuild Debug configuration
echo.
echo   launch_ucxclient-x64.cmd all
echo       Build both Debug and Release configurations
echo.
echo   launch_ucxclient-x64.cmd sign 1234567890ABCDEF...
echo       Sign Release build and save to ucxclient-x64-signed.exe
echo.
echo NOTES:
echo   - Release is the default (optimized, no debug symbols)
echo   - Debug is for development/troubleshooting (symbols, no optimization)
echo   - First launch auto-configures CMake if needed
echo   - Auto-builds if executable is missing or outdated
echo   - Auto-copies FTDI DLL when building
echo   - Settings file stored in workspace root directory
echo.
exit /b 0
