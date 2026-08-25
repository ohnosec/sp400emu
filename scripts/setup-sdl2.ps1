[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$sdlVersion = '2.32.2'
$archiveName = "SDL2-devel-$sdlVersion-VC.zip"
$archiveSha256 = '85F13B84C73C044CFF543AB98D9DBDA252DD757B8202AA63FBA02A9A162736A8'
$dllSha256 = '4EC9EEFA5B68824A79CE3B2DC66377B0450A19A8A524668C049FAB38BB923E70'
$sourceUrl = "https://www.libsdl.org/release/$archiveName"
$projectRoot = Split-Path -Parent $PSScriptRoot
$dependencyRoot = Join-Path $projectRoot 'third_party'
$archivePath = Join-Path $dependencyRoot $archiveName
$installPath = Join-Path $dependencyRoot "SDL2-$sdlVersion"
$headerPath = Join-Path $installPath 'include\SDL.h'
$libraryPath = Join-Path $installPath 'lib\x64\SDL2.lib'
$dllPath = Join-Path $installPath 'lib\x64\SDL2.dll'

if ((Test-Path -LiteralPath $headerPath) -and
    (Test-Path -LiteralPath $libraryPath) -and
    (Test-Path -LiteralPath $dllPath)) {
    $actualDllSha256 = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash
    if ($actualDllSha256 -ne $dllSha256) {
        throw "Installed SDL2.dll hash mismatch. Expected $dllSha256, got $actualDllSha256."
    }
    Write-Host "SDL2 $sdlVersion development files are already installed."
    return
}

New-Item -ItemType Directory -Force -Path $dependencyRoot | Out-Null

if (-not (Test-Path -LiteralPath $archivePath)) {
    Write-Host "Downloading $sourceUrl"
    Invoke-WebRequest -Uri $sourceUrl -OutFile $archivePath
}

$actualSha256 = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
if ($actualSha256 -ne $archiveSha256) {
    throw "SDL2 archive hash mismatch. Expected $archiveSha256, got $actualSha256."
}

Write-Host "Extracting $archiveName"
Expand-Archive -LiteralPath $archivePath -DestinationPath $dependencyRoot -Force

if (-not (Test-Path -LiteralPath $headerPath) -or
    -not (Test-Path -LiteralPath $libraryPath) -or
    -not (Test-Path -LiteralPath $dllPath)) {
    throw "SDL2 development archive did not contain the expected x64 files."
}

$actualDllSha256 = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash
if ($actualDllSha256 -ne $dllSha256) {
    throw "Extracted SDL2.dll hash mismatch. Expected $dllSha256, got $actualDllSha256."
}

Write-Host "SDL2 $sdlVersion development files installed at $installPath"
