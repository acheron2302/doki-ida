# Build (optional) + deploy the plugin and its assets for local testing.
#
#   tools\deploy.ps1                          # deploy DLL + assets + definitions
#   tools\deploy.ps1 -Build                   # cmake --build first
#   tools\deploy.ps1 -DeployToIdaSdkPlugins   # also copy DLL to $env:IDASDK\plugins
#
# Stages:
#   <IDADIR>\plugins\doki_theme.dll
#   $IDAUSR\doki-theme\definitions\*.json
#   $IDAUSR\doki-theme\assets\stickers\*.png
#   $IDAUSR\doki-theme\assets\wallpapers\*.png
#   [$env:IDASDK\plugins\doki_theme.dll] (only with -DeployToIdaSdkPlugins)
param([switch]$Build, [switch]$DeployToIdaSdkPlugins)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$idadir = $env:IDADIR
if (-not $idadir) { $idadir = "D:\tools\IDA 9.4" }
$idausr = $env:IDAUSR
if (-not $idausr) { $idausr = Join-Path $env:APPDATA "Hex-Rays\IDA Pro" }

if ($Build) {
  $cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  & $cmake --build (Join-Path $repoRoot "build") --config Release
}

# Locate the freshly built DLL (ida-cmake deploys to the SDK bin/plugins).
$dll = Join-Path $env:IDASDK "src\bin\plugins\doki_theme.dll"
if (-not (Test-Path $dll)) { $dll = Join-Path $repoRoot "build\Release\doki_theme.dll" }
Copy-Item $dll (Join-Path $idadir "plugins\doki_theme.dll") -Force
Write-Output "deployed DLL -> $idadir\plugins"

if ($DeployToIdaSdkPlugins) {
  if (-not $env:IDASDK) { throw "IDASDK env var is required for -DeployToIdaSdkPlugins" }
  $sdkPlugins = Join-Path $env:IDASDK "plugins"
  New-Item -ItemType Directory -Force $sdkPlugins | Out-Null
  Copy-Item $dll (Join-Path $sdkPlugins "doki_theme.dll") -Force
  Write-Output "deployed DLL -> $sdkPlugins"
}

$root = Join-Path $idausr "doki-theme"
$defs = Join-Path $root "definitions"
$stk  = Join-Path $root "assets\stickers"
$wall = Join-Path $root "assets\wallpapers"
New-Item -ItemType Directory -Force $defs, $stk, $wall | Out-Null
Copy-Item (Join-Path $repoRoot "definitions\*.json") $defs -Force
Copy-Item (Join-Path $repoRoot "assets\stickers\*.png") $stk -Force
if (Test-Path (Join-Path $repoRoot "assets\wallpapers")) {
  Copy-Item (Join-Path $repoRoot "assets\wallpapers\*.png") $wall -Force
}
Write-Output "deployed definitions + assets -> $root"
