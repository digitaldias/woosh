# Woosh - Release Build Script
# Usage: .\build-release.ps1 [-Deploy] [-SetupVcpkg] [-Test] [-Jobs <n>] [-Clean]

param(
    [switch]$Deploy,
    [switch]$SetupVcpkg,
    [switch]$Test,
    [switch]$Clean,
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

# Auto-detect CPU cores if not specified
if ($Jobs -eq 0) {
    $Jobs = [Environment]::ProcessorCount
    if (-not $Jobs) { $Jobs = 8 }
}

$startTime = Get-Date
Write-Host "=== Woosh Release Build ===" -ForegroundColor Cyan
Write-Host "Parallel jobs: $Jobs cores" -ForegroundColor Gray

# Clean build if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build" -ErrorAction SilentlyContinue
}

# Common Qt search paths
$qtSearchPaths = @("C:\Qt", "D:\Qt", "$env:USERPROFILE\Qt")

# Find or setup vcpkg
$vcpkgRoot = $null
$vcpkgToolchain = $null

$vcpkgLocations = @(
    $env:VCPKG_ROOT,
    "$PSScriptRoot\vcpkg",
    "C:\vcpkg",
    "C:\dev\vcpkg",
    "$env:USERPROFILE\vcpkg"
)

foreach ($loc in $vcpkgLocations) {
    if ($loc -and (Test-Path "$loc\scripts\buildsystems\vcpkg.cmake")) {
        $vcpkgRoot = $loc
        $vcpkgToolchain = "$loc\scripts\buildsystems\vcpkg.cmake"
        break
    }
}

if (-not $vcpkgRoot -and $SetupVcpkg) {
    Write-Host "Setting up vcpkg in project directory..." -ForegroundColor Yellow
    $vcpkgRoot = "$PSScriptRoot\vcpkg"
    
    if (-not (Test-Path $vcpkgRoot)) {
        git clone https://github.com/microsoft/vcpkg.git $vcpkgRoot
    }
    
    if (-not (Test-Path "$vcpkgRoot\vcpkg.exe")) {
        Push-Location $vcpkgRoot
        .\bootstrap-vcpkg.bat -disableMetrics
        Pop-Location
    }
    
    $vcpkgToolchain = "$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"
    Write-Host "vcpkg set up at: $vcpkgRoot" -ForegroundColor Green
}

if (-not $vcpkgRoot) {
    Write-Host "Warning: vcpkg not found. Run with -SetupVcpkg to set it up." -ForegroundColor Yellow
}

# Find Qt installation (prefer MSVC over MinGW)
$qtPath = $null
if ($env:CMAKE_PREFIX_PATH) {
    $qtPath = $env:CMAKE_PREFIX_PATH
} else {
    foreach ($basePath in $qtSearchPaths) {
        if (Test-Path $basePath) {
            $qtVersions = Get-ChildItem $basePath -Directory -ErrorAction SilentlyContinue | 
                Where-Object { $_.Name -match '^\d+\.\d+' } | 
                Sort-Object { [version]($_.Name -replace '[^\d.].*$', '') } -Descending
            
            foreach ($ver in $qtVersions) {
                # Strongly prefer MSVC 2022, then MSVC 2019, then MinGW
                $compilerPath = Get-ChildItem $ver.FullName -Directory -ErrorAction SilentlyContinue | 
                    Where-Object { $_.Name -match 'msvc2022_64' } | Select-Object -First 1
                
                if (-not $compilerPath) {
                    $compilerPath = Get-ChildItem $ver.FullName -Directory -ErrorAction SilentlyContinue | 
                        Where-Object { $_.Name -match 'msvc\d+_64' } | Select-Object -First 1
                }
                
                if (-not $compilerPath) {
                    $compilerPath = Get-ChildItem $ver.FullName -Directory -ErrorAction SilentlyContinue | 
                        Where-Object { $_.Name -match 'mingw' } | Select-Object -First 1
                }
                
                if ($compilerPath) {
                    $qtPath = $compilerPath.FullName
                    break
                }
            }
            if ($qtPath) { break }
        }
    }
}

if (-not $qtPath) {
    Write-Host "Error: Qt not found. Install Qt 6 or set CMAKE_PREFIX_PATH." -ForegroundColor Red
    exit 1
}

$useMinGW = $qtPath -match 'mingw'
Write-Host "Using Qt: $qtPath" -ForegroundColor Gray

# Configure if needed
$needsConfigure = -not (Test-Path "build\CMakeCache.txt")

if ($needsConfigure) {
    Write-Host "Configuring CMake (Release)..." -ForegroundColor Yellow
    
    $cmakeArgs = @("-B", "build", "-DCMAKE_BUILD_TYPE=Release")
    
    if ($useMinGW) {
        $cmakeArgs += @("-G", "MinGW Makefiles")
        if ($vcpkgToolchain) {
            $cmakeArgs += "-DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic"
        }
    }
    
    if ($vcpkgToolchain) {
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
    }
    
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$qtPath"
    
    cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build
Write-Host "Building with $Jobs parallel jobs..." -ForegroundColor Yellow
cmake --build build --config Release --parallel $Jobs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$elapsed = (Get-Date) - $startTime
Write-Host ""
Write-Host "Build successful! ($('{0:mm\:ss}' -f $elapsed))" -ForegroundColor Green

$exePath = if ($useMinGW) { "build\Woosh.exe" } else { "build\Release\Woosh.exe" }
Write-Host "Executable: $exePath" -ForegroundColor Gray

# Run tests if requested
if ($Test) {
    Write-Host "Running tests..." -ForegroundColor Yellow
    ctest --test-dir build -C Release --output-on-failure --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { 
        Write-Host "Tests failed!" -ForegroundColor Red
        exit $LASTEXITCODE 
    }
    Write-Host "All tests passed!" -ForegroundColor Green
}

# Deploy Qt dependencies if requested
if ($Deploy) {
    Write-Host "Running windeployqt..." -ForegroundColor Yellow
    windeployqt --release --no-translations $exePath
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Qt dependencies deployed!" -ForegroundColor Green
}
