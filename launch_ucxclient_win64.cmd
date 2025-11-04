@echo off
REM Launch script for ucxclient_win64

setlocal enabledelayedexpansion

echo ===================================
echo ucxclient_win64 Launcher
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
REM Auto-select priority: 1) Signed Release, 2) Signed Debug, 3) Release, 4) Debug, 5) Build Debug
REM User can override with: release, debug, release_signed, debug_signed, -r, -d, -rs, -ds
set CONFIG=Debug
set USER_SPECIFIED_CONFIG=0

REM Check if user specified a configuration
if /i "%1"=="release" (
    set CONFIG=Release
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="Release" (
    set CONFIG=Release
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="-r" (
    set CONFIG=Release
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="debug" (
    set CONFIG=Debug
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="Debug" (
    set CONFIG=Debug
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="-d" (
    set CONFIG=Debug
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="release_signed" (
    set CONFIG=Release_Signed
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="Release_Signed" (
    set CONFIG=Release_Signed
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="-rs" (
    set CONFIG=Release_Signed
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="debug_signed" (
    set CONFIG=Debug_Signed
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="Debug_Signed" (
    set CONFIG=Debug_Signed
    set USER_SPECIFIED_CONFIG=1
)
if /i "%1"=="-ds" (
    set CONFIG=Debug_Signed
    set USER_SPECIFIED_CONFIG=1
)

REM If no config specified, use priority order:
REM 1. Signed Release (production)
REM 2. Signed Debug (testing with symbols)
REM 3. Release (unsigned development)
REM 4. Debug (unsigned development with symbols)
REM 5. Build Debug if none exist
if "%USER_SPECIFIED_CONFIG%"=="0" (
    if exist "build\Release_Signed\ucxclient_win64.exe" (
        set CONFIG=Release_Signed
        echo [Auto-select] Using existing signed Release build ^(priority 1^)...
        echo.
    ) else if exist "build\Debug_Signed\ucxclient_win64.exe" (
        set CONFIG=Debug_Signed
        echo [Auto-select] Using existing signed Debug build ^(priority 2^)...
        echo.
    ) else if exist "build\Release\ucxclient_win64.exe" (
        set CONFIG=Release
        echo [Auto-select] Using existing Release build ^(priority 3^)...
        echo.
    ) else if exist "build\Debug\ucxclient_win64.exe" (
        set CONFIG=Debug
        echo [Auto-select] Using existing Debug build ^(priority 4^)...
        echo.
    ) else (
        set CONFIG=Debug
        echo [Auto-select] No existing build found. Will build Debug configuration...
        echo.
    )
) else (
    echo Using !CONFIG! configuration ^(user specified^)...
    echo.
)

REM Set build directory
set BUILD_DIR=build\!CONFIG!
set FTDI_DLL=examples\ftdi\ftd2xx64.dll

REM Check if executable exists, build if not
if not exist "!BUILD_DIR!\ucxclient_win64.exe" (
    echo Executable not found. Auto-building !CONFIG!...
    echo.
    
    REM Configure if needed
    if not exist "build\ucxclient_win64.vcxproj" (
        echo Configuring CMake...
        cmake -S . -B build
        if errorlevel 1 (
            echo ERROR: CMake configuration failed!
            exit /b 1
        )
    )
    
    REM Build
    echo Building ucxclient_win64.exe in !CONFIG! configuration...
    cmake --build build --config !CONFIG! --target ucxclient_win64
    if errorlevel 1 (
        echo ERROR: Build failed!
        exit /b 1
    )
    echo Build successful!
) else (
    echo Executable found. Skipping build.
)

REM Copy FTDI DLL if needed
if not exist "!BUILD_DIR!\ftd2xx64.dll" (
    if exist "!FTDI_DLL!" (
        echo Copying FTDI DLL...
        copy /Y "!FTDI_DLL!" "!BUILD_DIR!\" >nul
    )
)

REM Check if executable exists after build
if not exist "!BUILD_DIR!\ucxclient_win64.exe" (
    echo ERROR: ucxclient_win64.exe not found in !BUILD_DIR!
    exit /b 1
)

REM Launch the application
echo.
echo ===================================
echo Launching ucxclient_win64.exe...
echo ===================================
echo.

cd "!BUILD_DIR!"
ucxclient_win64.exe %2 %3 %4 %5 %6 %7 %8 %9

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
if not exist "build\ucxclient_win64.vcxproj" (
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
cmake --build build --config %REBUILD_CONFIG% --target ucxclient_win64
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
if not exist "build\ucxclient_win64.vcxproj" (
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
cmake --build build --config Debug --target ucxclient_win64
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
cmake --build build --config Release --target ucxclient_win64
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
echo   build\Debug\ucxclient_win64.exe
echo   build\Release\ucxclient_win64.exe
echo.
exit /b 0

REM ===================================
REM Sign command
REM ===================================
:sign
echo ===================================
echo Code Signing ucxclient_win64.exe
echo ===================================
echo.

REM Determine configuration
set SIGN_CONFIG=Release
if /i "%2"=="debug" set SIGN_CONFIG=Debug
if /i "%2"=="Debug" set SIGN_CONFIG=Debug

set SIGN_EXE=build\!SIGN_CONFIG!\ucxclient_win64.exe
set SIGN_EXE_SIGNED=build\!SIGN_CONFIG!\ucxclient_win64_signed.exe
set CERT_THUMBPRINT=%3

REM Check if executable exists
if not exist "!SIGN_EXE!" (
    echo [ERROR] Executable not found: !SIGN_EXE!
    echo.
    echo Build the Release version first:
    echo   launch_ucxclient_win64.cmd rebuild release
    echo.
    exit /b 1
)

REM Check if thumbprint provided
if not defined CERT_THUMBPRINT (
    echo [ERROR] Certificate thumbprint required!
    echo.
    echo USAGE:
    echo   launch_ucxclient_win64.cmd sign [config] [thumbprint]
    echo.
    echo EXAMPLES:
    echo   launch_ucxclient_win64.cmd sign release 1234567890ABCDEF...
    echo   launch_ucxclient_win64.cmd sign debug 1234567890ABCDEF...
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
echo Renaming signed executable...
echo ===================================
echo.

REM Rename the signed executable
move /Y "!SIGN_EXE!" "!SIGN_EXE_SIGNED!"
if errorlevel 1 (
    echo [ERROR] Failed to rename signed executable!
    exit /b 1
)

echo Renamed: !SIGN_EXE!
echo     To: !SIGN_EXE_SIGNED!
echo.

echo ===================================
echo Signing completed successfully!
echo ===================================
echo Signed executable: !SIGN_EXE_SIGNED!
echo.

exit /b 0

REM ===================================
REM Help command
REM ===================================
:help
echo.
echo USAGE:
echo   launch_ucxclient_win64.cmd [command^|config] [arguments]
echo.
echo COMMANDS:
echo   clean [config]        Deep clean build artifacts
echo                         Removes ALL .obj, .exe, .pdb, .ilk files
echo                         - No config = clean all (Debug + Release)
echo                         - 'debug' or 'release' = clean specific config
echo.
echo   rebuild [config]      Deep clean and rebuild from scratch
echo                         Forces recompilation of ALL source files
echo                         - No config = rebuild Debug
echo                         - 'release' = rebuild Release
echo.
echo   all                   Build both Debug and Release configurations
echo                         Builds all configurations from current state
echo                         (use 'clean' first for a fresh build)
echo.
echo   sign [config] [thumbprint]
echo                         Code sign the executable with certificate
echo                         - config: 'debug' or 'release' (default: release)
echo                         - thumbprint: Certificate thumbprint (required)
echo                         Example: sign release 1234567890ABCDEF...
echo.
echo   help / --help / -h    Show this help message
echo.
echo CONFIGURATIONS:
echo   (none)                Auto-select by priority:
echo                           1. Release_Signed  (signed production)
echo                           2. Debug_Signed    (signed with symbols)
echo                           3. Release         (unsigned)
echo                           4. Debug           (unsigned)
echo                           5. Build Debug if none exist
echo   debug / Debug / -d    Launch Debug build
echo   release / Release / -r
echo                         Launch Release build
echo   debug_signed / Debug_Signed / -ds
echo                         Launch signed Debug build
echo   release_signed / Release_Signed / -rs
echo                         Launch signed Release build
echo.
echo EXAMPLES:
echo   launch_ucxclient_win64.cmd
echo       Auto-select best available build (signed release preferred)
echo.
echo   launch_ucxclient_win64.cmd debug
echo       Launch Debug build (auto-builds if needed)
echo.
echo   launch_ucxclient_win64.cmd release
echo       Launch Release build (auto-builds if needed)
echo.
echo   launch_ucxclient_win64.cmd Debug COM4
echo       Launch Debug build and pass COM4 to the app
echo.
echo   launch_ucxclient_win64.cmd clean
echo       Clean all configurations (Debug and Release)
echo.
echo   launch_ucxclient_win64.cmd clean debug
echo       Clean only Debug configuration
echo.
echo   launch_ucxclient_win64.cmd rebuild
echo       Clean and rebuild Debug configuration
echo.
echo   launch_ucxclient_win64.cmd rebuild release
echo       Clean and rebuild Release configuration
echo.
echo   launch_ucxclient_win64.cmd all
echo       Build both Debug and Release configurations
echo.
echo   launch_ucxclient_win64.cmd sign release 1234567890ABCDEF...
echo       Sign Release build and rename to ucxclient_win64_signed.exe
echo.
echo NOTES:
echo   - First launch auto-configures CMake if needed
echo   - Auto-builds if executable is missing
echo   - Auto-copies FTDI DLL when building
echo   - Settings file stored next to executable
echo.
exit /b 0
