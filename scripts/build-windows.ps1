[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateRange(0, 256)]
    [int]$ParallelJobs = 0
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$romPath = Join-Path $projectRoot 'sp400_6805.bin'

if (-not (Test-Path -LiteralPath $romPath)) {
    throw "sp400_6805.bin is missing from the project root: $projectRoot"
}

& (Join-Path $PSScriptRoot 'setup-sdl2.ps1')

$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw 'Visual Studio Installer (vswhere.exe) was not found.'
}

$visualStudioPath = & $vswherePath -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($visualStudioPath)) {
    throw 'Visual Studio with the MSVC x64 build tools was not found.'
}

$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if ($null -ne $cmakeCommand) {
    $cmakePath = $cmakeCommand.Source
} else {
    $cmakePath = Join-Path $visualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}

if (-not (Test-Path -LiteralPath $cmakePath)) {
    throw "CMake was not found on PATH or in Visual Studio: $cmakePath"
}

$buildPath = Join-Path $projectRoot 'build\windows'
$sdlRoot = Join-Path $projectRoot 'third_party\SDL2-2.32.2'

& $cmakePath -S $projectRoot -B $buildPath `
    -G 'Visual Studio 17 2022' -A x64 "-DSDL2_ROOT=$sdlRoot"
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

$buildArguments = @('--build', $buildPath, '--config', $Configuration, '--parallel')
if ($ParallelJobs -gt 0) {
    $buildArguments += $ParallelJobs
}

& $cmakePath @buildArguments
if ($LASTEXITCODE -ne 0) {
    throw "MSVC build failed with exit code $LASTEXITCODE."
}

$executablePath = Join-Path $buildPath "$Configuration\sp400.exe"
if (-not (Test-Path -LiteralPath $executablePath)) {
    throw "Build completed without producing the expected executable: $executablePath"
}

Write-Host "Built $executablePath"
