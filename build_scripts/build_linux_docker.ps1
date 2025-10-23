#!/usr/bin/env pwsh
# Build Linux library using Docker from Windows

Write-Host "Building Linux library using Docker..." -ForegroundColor Cyan
Write-Host ""

# Check if Docker is available
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "Error: Docker is not installed or not in PATH" -ForegroundColor Red
    Write-Host "Please install Docker Desktop from: https://www.docker.com/products/docker-desktop" -ForegroundColor Yellow
    exit 1
}

# Check if Docker is running
docker ps 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Docker is not running" -ForegroundColor Red
    Write-Host "Please start Docker Desktop" -ForegroundColor Yellow
    exit 1
}

Write-Host "✓ Docker is available" -ForegroundColor Green

# Build the Docker image
Write-Host ""
Write-Host "Building Docker image..." -ForegroundColor Cyan
docker build -f Dockerfile.build -t ucxclient-builder .

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Failed to build Docker image" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Docker image built" -ForegroundColor Green

# Build the Linux library
Write-Host ""
Write-Host "Building Linux library (libucxclient.so)..." -ForegroundColor Cyan

# Remove old build directory
if (Test-Path "build-linux") {
    Write-Host "Removing old build-linux directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build-linux
}

# Run the build in Docker
docker run --rm -v "${PWD}:/workspace" ucxclient-builder bash -c @"
mkdir -p build-linux && \
cd build-linux && \
cmake .. && \
cmake --build . && \
echo '' && \
echo '=== Build Complete ===' && \
ls -lh libucxclient.so 2>/dev/null || ls -lh */libucxclient.so 2>/dev/null || echo 'Library location:' && find . -name 'libucxclient.so' -exec ls -lh {} \;
"@

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "Error: Build failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓ Build successful!" -ForegroundColor Green

# Check if the library was created
$libPath = Get-ChildItem -Path "build-linux" -Filter "libucxclient.so" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1

if ($libPath) {
    Write-Host ""
    Write-Host "Linux library created: $($libPath.FullName)" -ForegroundColor Cyan
    Write-Host "Size: $([math]::Round($libPath.Length / 1KB, 2)) KB" -ForegroundColor Cyan
    
    # Optionally copy to build directory for consistency
    $targetDir = "build"
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir | Out-Null
    }
    
    Write-Host ""
    Write-Host "Copying to build/libucxclient.so for consistency..." -ForegroundColor Yellow
    Copy-Item $libPath.FullName -Destination "$targetDir/libucxclient.so" -Force
    Write-Host "✓ Copied to build/libucxclient.so" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "Warning: Could not locate libucxclient.so" -ForegroundColor Yellow
    Write-Host "Check build-linux directory manually" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done! You can now use the Linux library with WSL or on a Linux system." -ForegroundColor Green
