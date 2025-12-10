# Woosh - Release Build Script
# Usage: .\build-release.ps1 [-Deploy] [-SetupVcpkg] [-Test] [-Jobs <n>] [-Quiet]

param(
    [switch]$Deploy,
    [switch]$SetupVcpkg,
    [switch]$Test,
    [switch]$Quiet,  # Only show warnings and errors
    [int]$Jobs = 0   # 0 = auto-detect
)

$ErrorActionPreference = "Stop"

# Auto-detect CPU cores if not specified
if ($Jobs -eq 0) {
    $Jobs = (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors
    if (-not $Jobs) { $Jobs = $env:NUMBER_OF_PROCESSORS }
    if (-not $Jobs) { $Jobs = 8 }  # Fallback
}

$startTime = Get-Date
if (-not $Quiet) {
    Write-Host "=== Woosh Release Build ===" -ForegroundColor Cyan
    Write-Host "Parallel jobs: $Jobs cores" -ForegroundColor Gray
}

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
    Write-Host "Configuring CMake (Release)..." -ForegroundColor Yellow
    
    $cmakeArgs = @("-B", "build", "-DCMAKE_BUILD_TYPE=Release", "-Wno-dev")
    
    # Use Ninja generator for faster builds (or MinGW Makefiles as fallback)
    if ($useMinGW) {
        if (Get-Command ninja -ErrorAction SilentlyContinue) {
            Write-Host "Using Ninja generator (fast parallel builds)" -ForegroundColor Gray
            $cmakeArgs += "-G"
            $cmakeArgs += "Ninja"
        } else {
            Write-Host "Using MinGW Makefiles generator (install Ninja for faster builds)" -ForegroundColor Gray
            $cmakeArgs += "-G"
            $cmakeArgs += "MinGW Makefiles"
        }
        
        # Set vcpkg triplet for MinGW
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
    
    if ($Quiet) {
        $output = cmake @cmakeArgs 2>&1
        $exitCode = $LASTEXITCODE
        $output | Where-Object { $_ -match 'warning|error|fatal' } | ForEach-Object {
            if ($_ -match 'error|fatal') { Write-Host $_ -ForegroundColor Red }
            else { Write-Host $_ -ForegroundColor Yellow }
        }
        if ($exitCode -ne 0) { exit $exitCode }
    } else {
        cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

# Build with parallel jobs
if ($Quiet) {
    Write-Host "Building (quiet mode - warnings/errors only)..." -ForegroundColor Yellow
    $output = cmake --build build --config Release --parallel $Jobs 2>&1
    $exitCode = $LASTEXITCODE
    # Filter to only show warnings and errors
    $output | Where-Object { $_ -match 'warning|error|failed|fatal' } | ForEach-Object {
        if ($_ -match 'error|fatal|failed') {
            Write-Host $_ -ForegroundColor Red
        } else {
            Write-Host $_ -ForegroundColor Yellow
        }
    }
    if ($exitCode -ne 0) { exit $exitCode }
} else {
    Write-Host "Building with $Jobs parallel jobs..." -ForegroundColor Yellow
    cmake --build build --config Release --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$elapsed = (Get-Date) - $startTime
Write-Host ""
Write-Host "Build successful! ($('{0:mm\:ss}' -f $elapsed))" -ForegroundColor Green

# Show executable path based on generator
if ($useMinGW) {
    $exePath = "build\Woosh.exe"
} else {
    $exePath = "build\Release\Woosh.exe"
}
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
