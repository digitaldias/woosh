# Woosh - Debug Build Script
# Usage: .\build-debug.ps1

$ErrorActionPreference = "Stop"

Write-Host "=== Woosh Debug Build ===" -ForegroundColor Cyan

# Configure if build directory doesn't exist or CMakeLists.txt changed
if (-not (Test-Path "build\CMakeCache.txt")) {
    Write-Host "Configuring CMake (Debug)..." -ForegroundColor Yellow
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build build --config Debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: build\Debug\Woosh.exe" -ForegroundColor Gray

# Optional: Deploy Qt dependencies
$deployQt = Read-Host "Deploy Qt dependencies? (y/N)"
if ($deployQt -eq 'y' -or $deployQt -eq 'Y') {
    Write-Host "Running windeployqt..." -ForegroundColor Yellow
    windeployqt --debug build\Debug\Woosh.exe
    Write-Host "Qt dependencies deployed!" -ForegroundColor Green
}

