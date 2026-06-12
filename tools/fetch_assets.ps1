# Re-pull doki sticker assets from doki-theme-assets into assets/stickers/.
# Add entries to $map to bundle more characters. Run from anywhere.
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$dst = Join-Path $repoRoot "assets\stickers"
New-Item -ItemType Directory -Force $dst | Out-Null

$base = "https://raw.githubusercontent.com/doki-theme/doki-theme-assets/master/stickers/vscode"
$map = @{
  "rem.png"               = "$base/reZero/rem/rem.png"
  "zero_two_obsidian.png" = "$base/franxx/zeroTwo/obsidian/zero_two_obsidian.png"
  "zero_two_sakura.png"   = "$base/franxx/zeroTwo/sakura/zero_two_sakura.png"
  "darkness_dark.png"     = "$base/konoSuba/darkness/dark/darkness_dark.png"
  "darkness_light.png"    = "$base/konoSuba/darkness/light/darkness_light.png"
}
foreach ($k in $map.Keys) {
  Invoke-WebRequest -Uri $map[$k] -OutFile (Join-Path $dst $k) -UseBasicParsing
  Write-Output "fetched $k"
}
Write-Output "done -> $dst"
