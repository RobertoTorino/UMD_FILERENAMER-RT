param(
    [string]$QtRoot = "C:/Qt/6.10.2/msvc2022_64",
    [string]$CMakeExe = "C:/Program Files/CMake/bin/cmake.exe",
    [string]$VsBuildToolsRoot = "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools",
    [string]$Generator = "Ninja",
    [string]$BuildType = "Release",
    [switch]$Clean,
    [switch]$Deploy
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$ExeName = "UMD_FileRenamer.exe"
$ExePath = Join-Path $BuildDir $ExeName
$VsDevCmd = Join-Path $VsBuildToolsRoot "Common7\Tools\VsDevCmd.bat"
$QtDeployTool = Join-Path $QtRoot "bin\windeployqt6.exe"
$Qt6Dir = Join-Path $QtRoot "lib\cmake\Qt6"

function Assert-Path {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path $Path)) {
        throw "$Label not found: $Path"
    }
}

function Normalize-ProcessPath {
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $parts = New-Object System.Collections.Generic.List[string]

    foreach ($entry in ($env:Path -split ';')) {
        $trimmed = $entry.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) {
            continue
        }

        if ($seen.Add($trimmed)) {
            $parts.Add($trimmed)
        }
    }

    $env:Path = ($parts -join ';')
}

function Import-VsDevEnvironment {
    param([string]$BatchFile)

    $tempFile = [System.IO.Path]::GetTempFileName()

    try {
        $cmd = '"{0}" -arch=x64 -host_arch=x64 >nul && set > "{1}"' -f $BatchFile, $tempFile
        & cmd.exe /s /c $cmd
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to initialize the Visual Studio build environment."
        }

        foreach ($line in Get-Content -Path $tempFile) {
            if ($line -match "^([^=]+)=(.*)$") {
                [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
            }
        }
    }
    finally {
        if (Test-Path $tempFile) {
            Remove-Item -Path $tempFile -Force -ErrorAction SilentlyContinue
        }
    }
}

Assert-Path -Path $CMakeExe -Label "CMake"
Assert-Path -Path $QtRoot -Label "Qt root"
Assert-Path -Path $Qt6Dir -Label "Qt6 CMake package"
Assert-Path -Path $VsDevCmd -Label "VsDevCmd"

Normalize-ProcessPath
Import-VsDevEnvironment -BatchFile $VsDevCmd

if ($Clean -and (Test-Path $BuildDir)) {
    Remove-Item -Path $BuildDir -Recurse -Force
}

$cmakeArgs = @(
    "-S", $ProjectRoot,
    "-B", $BuildDir,
    "-G", $Generator,
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_PREFIX_PATH=$QtRoot",
    "-DQt6_DIR=$Qt6Dir"
)

Write-Host "Configuring with Qt: $QtRoot" -ForegroundColor Cyan
& $CMakeExe @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host "Building $ExeName" -ForegroundColor Cyan
& $CMakeExe --build $BuildDir --config $BuildType
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

Assert-Path -Path $ExePath -Label "Built executable"

if ($Deploy) {
    Assert-Path -Path $QtDeployTool -Label "windeployqt6"
    Write-Host "Deploying Qt runtime" -ForegroundColor Cyan
    & $QtDeployTool $ExePath
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt6 failed with exit code $LASTEXITCODE."
    }
}

Write-Host "Build complete: $ExePath" -ForegroundColor Green
