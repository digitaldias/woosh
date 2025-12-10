# Woosh - Release Build Script
# Usage: .\build-release.ps1

$ErrorActionPreference = "Stop"

Write-Host "=== Woosh Release Build ===" -ForegroundColor Cyan

# Configure if build directory doesn't exist or CMakeLists.txt changed
if (-not (Test-Path "build\CMakeCache.txt")) {
    Write-Host "Configuring CMake (Release)..." -ForegroundColor Yellow
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: build\Release\Woosh.exe" -ForegroundColor Gray

# Optional: Deploy Qt dependencies
$deployQt = Read-Host "Deploy Qt dependencies? (y/N)"
if ($deployQt -eq 'y' -or $deployQt -eq 'Y') {
    Write-Host "Running windeployqt..." -ForegroundColor Yellow
    windeployqt --release --no-translations build\Release\Woosh.exe
    Write-Host "Qt dependencies deployed!" -ForegroundColor Green
}

# Optional: Run tests
$runTests = Read-Host "Run tests? (y/N)"
if ($runTests -eq 'y' -or $runTests -eq 'Y') {
    Write-Host "Running tests..." -ForegroundColor Yellow
    ctest --test-dir build -C Release --output-on-failure
}

