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

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$QtBuildScript = Join-Path $RepoRoot "qt\build.ps1"

if (-not (Test-Path $QtBuildScript)) {
    throw "Qt build script not found: $QtBuildScript"
}

$qtArgs = @{
    QtRoot = $QtRoot
    CMakeExe = $CMakeExe
    VsBuildToolsRoot = $VsBuildToolsRoot
    Generator = $Generator
    BuildType = $BuildType
}

if ($Clean) {
    $qtArgs.Clean = $true
}

if ($Deploy) {
    $qtArgs.Deploy = $true
}

& $QtBuildScript @qtArgs
