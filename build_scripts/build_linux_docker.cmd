@echo off
REM Build Linux library using Docker from Windows

echo Building Linux library using Docker...
echo.

REM Check if Docker is available
docker --version >nul 2>&1
if errorlevel 1 (
    echo Error: Docker is not installed or not in PATH
    echo Please install Docker Desktop from: https://www.docker.com/products/docker-desktop
    exit /b 1
)

REM Check if Docker is running
docker ps >nul 2>&1
if errorlevel 1 (
    echo Error: Docker is not running
    echo Please start Docker Desktop
    exit /b 1
)

echo Docker is available
echo.

REM Build the Docker image
echo Building Docker image...
docker build -f Dockerfile.build -t ucxclient-builder .
if errorlevel 1 (
    echo Error: Failed to build Docker image
    exit /b 1
)

echo Docker image built successfully
echo.

REM Build the Linux library
echo Building Linux library (libucxclient.so)...
docker run --rm -v "%CD%":/workspace ucxclient-builder bash -c "mkdir -p build-linux && cd build-linux && cmake .. && cmake --build . && echo '' && echo '=== Build Complete ===' && find . -name 'libucxclient.so' -exec ls -lh {} \;"

if errorlevel 1 (
    echo.
    echo Error: Build failed
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Linux library created in build-linux/
echo You can now use it with WSL or on a Linux system.
echo.

pause
