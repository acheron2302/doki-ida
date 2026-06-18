# Developer utility: pre-warm the local cache for the bundled themes.
#
# The runtime (src/doki/assets.cpp) downloads on demand from the
# doki-theme CDN. This script is provided for offline development /
# air-gapped testing: it downloads the same assets into a target cache
# directory using the same authoritative URLs the runtime uses.
#
# Usage:
#   tools\fetch_assets.ps1                       # cache under $IDAUSR\doki-theme\cache
#   tools\fetch_assets.ps1 -OutDir <path>        # cache under <path>\cache
param(
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutDir) {
    $idausr = $env:IDAUSR
    if (-not $idausr) { $idausr = Join-Path $env:APPDATA "Hex-Rays\IDA Pro" }
    $OutDir = Join-Path $idausr "doki-theme"
}

$stickersDir = Join-Path $OutDir "cache\stickers"
$wallDir     = Join-Path $OutDir "cache\wallpapers"
New-Item -ItemType Directory -Force $stickersDir, $wallDir | Out-Null

$cdn = "https://doki.assets.unthrottled.io"
$stickers = @{
  "rem.png"               = "$cdn/stickers/vscode/reZero/rem/rem.png"
  "zero_two_obsidian.png" = "$cdn/stickers/vscode/franxx/zeroTwo/obsidian/zero_two_obsidian.png"
  "zero_two_sakura.png"   = "$cdn/stickers/vscode/franxx/zeroTwo/sakura/zero_two_sakura.png"
  "darkness_dark.png"     = "$cdn/stickers/vscode/konoSuba/darkness/dark/darkness_dark.png"
  "darkness_light.png"    = "$cdn/stickers/vscode/konoSuba/darkness/light/darkness_light.png"
}
$wallpapers = @{
  "rem.png"               = "$cdn/backgrounds/wallpapers/transparent/rem.png"
  "zero_two_obsidian.png" = "$cdn/backgrounds/wallpapers/transparent/zero_two_obsidian.png"
  "zero_two_sakura.png"   = "$cdn/backgrounds/wallpapers/transparent/zero_two_sakura.png"
  "darkness_dark.png"     = "$cdn/backgrounds/wallpapers/transparent/darkness_dark.png"
  "darkness_light.png"    = "$cdn/backgrounds/wallpapers/transparent/darkness_light.png"
}

foreach ($k in $stickers.Keys) {
  Invoke-WebRequest -Uri $stickers[$k] -OutFile (Join-Path $stickersDir $k) -UseBasicParsing
  Write-Output "fetched sticker $k"
}
foreach ($k in $wallpapers.Keys) {
  Invoke-WebRequest -Uri $wallpapers[$k] -OutFile (Join-Path $wallDir $k) -UseBasicParsing
  Write-Output "fetched wallpaper $k"
}
Write-Output "done -> $OutDir\cache"
