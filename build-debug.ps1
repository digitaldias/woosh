# Woosh - Debug Build Script
# Usage: .\build-debug.ps1 [-Deploy] [-SetupVcpkg]

param(
    [switch]$Deploy,
    [switch]$SetupVcpkg
)

$ErrorActionPreference = "Stop"

Write-Host "=== Woosh Debug Build ===" -ForegroundColor Cyan

# Common Qt search paths
$qtSearchPaths = @("C:\Qt", "D:\Qt", "$env:USERPROFILE\Qt")

# Find or setup vcpkg
$vcpkgRoot = $null
$vcpkgToolchain = $null

# Check common vcpkg locations
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

# Clone and bootstrap vcpkg if not found and requested
if (-not $vcpkgRoot) {
    if ($SetupVcpkg) {
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
    } else {
        Write-Host "Warning: vcpkg not found. Dependencies (libsndfile, mpg123) may not be available." -ForegroundColor Yellow
        Write-Host "Run with -SetupVcpkg to automatically clone and bootstrap vcpkg." -ForegroundColor Yellow
    }
}

# Find Qt installation
$qtPath = $null
if ($env:CMAKE_PREFIX_PATH) {
    $qtPath = $env:CMAKE_PREFIX_PATH
} else {
    foreach ($basePath in $qtSearchPaths) {
        if (Test-Path $basePath) {
            # Find latest Qt version with MSVC or MinGW
            $qtVersions = Get-ChildItem $basePath -Directory | Where-Object { $_.Name -match '^\d+\.\d+' } | Sort-Object Name -Descending
            foreach ($ver in $qtVersions) {
                # Prefer MSVC, fallback to MinGW
                $compilerPath = Get-ChildItem $ver.FullName -Directory | Where-Object { $_.Name -match 'msvc\d+_64' } | Select-Object -First 1
                if (-not $compilerPath) {
                    $compilerPath = Get-ChildItem $ver.FullName -Directory | Where-Object { $_.Name -match 'mingw_64' } | Select-Object -First 1
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

# Determine if using MinGW
$useMinGW = $qtPath -and $qtPath -match 'mingw'

# Add MinGW to PATH if needed
if ($useMinGW) {
    foreach ($searchPath in $qtSearchPaths) {
        if (Test-Path "$searchPath\Tools") {
            $mingwDirs = Get-ChildItem "$searchPath\Tools" -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match 'mingw' }
            if ($mingwDirs) {
                $mingwBin = Join-Path $mingwDirs[0].FullName "bin"
                if (Test-Path $mingwBin) {
                    Write-Host "Adding MinGW to PATH: $mingwBin" -ForegroundColor Gray
                    $env:PATH = "$mingwBin;$env:PATH"
                    break
                }
            }
        }
    }
}

# Detect if we need to reconfigure (CMakeLists.txt newer than cache)
$needsConfigure = $false
if (-not (Test-Path "build\CMakeCache.txt")) {
    $needsConfigure = $true
} else {
    $cacheTime = (Get-Item "build\CMakeCache.txt").LastWriteTime
    $cmakeTime = (Get-Item "CMakeLists.txt").LastWriteTime
    if ($cmakeTime -gt $cacheTime) {
        Write-Host "CMakeLists.txt changed, reconfiguring..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force build
        $needsConfigure = $true
    }
}

if ($needsConfigure) {
    Write-Host "Configuring CMake (Debug)..." -ForegroundColor Yellow
    
    $cmakeArgs = @("-B", "build", "-DCMAKE_BUILD_TYPE=Debug", "-Wno-dev")
    
    # Use MinGW generator if Qt is MinGW
    if ($useMinGW) {
        Write-Host "Using MinGW Makefiles generator" -ForegroundColor Gray
        $cmakeArgs += "-G"
        $cmakeArgs += "MinGW Makefiles"
        
        # Also set vcpkg triplet for MinGW
        if ($vcpkgToolchain) {
            $cmakeArgs += "-DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic"
        }
    }
    
    if ($vcpkgToolchain -and (Test-Path $vcpkgToolchain)) {
        Write-Host "Using vcpkg: $vcpkgRoot" -ForegroundColor Gray
        $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgToolchain"
    }
    
    if ($qtPath) {
        Write-Host "Using Qt: $qtPath" -ForegroundColor Gray
        $cmakeArgs += "-DCMAKE_PREFIX_PATH=$qtPath"
    } else {
        Write-Host "Error: Qt not found. Install Qt or set CMAKE_PREFIX_PATH." -ForegroundColor Red
        exit 1
    }
    
    cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build build --config Debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green

# Show executable path based on generator
if ($useMinGW) {
    Write-Host "Executable: build\Woosh.exe" -ForegroundColor Gray
} else {
    Write-Host "Executable: build\Debug\Woosh.exe" -ForegroundColor Gray
}

# Deploy Qt dependencies if requested
if ($Deploy) {
    Write-Host "Running windeployqt..." -ForegroundColor Yellow
    if ($useMinGW) {
        windeployqt --debug build\Woosh.exe
    } else {
        windeployqt --debug build\Debug\Woosh.exe
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Qt dependencies deployed!" -ForegroundColor Green
}
