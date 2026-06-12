# Stage a distributable package and zip it into dist/.
#
# Layout (drop the contents into your IDA install dir / $IDAUSR):
#   dist/doki-theme-ida/
#     plugins/doki_theme.dll
#     doki-theme/definitions/*.json
#     doki-theme/assets/stickers/*.png
#     ida-plugin.json
#     INSTALL.md
param([string]$Version = "0.1.0")

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $repoRoot "dist\doki-theme-ida"
$zip   = Join-Path $repoRoot "dist\doki-theme-ida-$Version.zip"

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force `
  (Join-Path $stage "plugins"),
  (Join-Path $stage "doki-theme\definitions"),
  (Join-Path $stage "doki-theme\assets\stickers") | Out-Null

$dll = Join-Path $env:IDASDK "src\bin\plugins\doki_theme.dll"
if (-not (Test-Path $dll)) { $dll = Join-Path $repoRoot "build\Release\doki_theme.dll" }
if (-not (Test-Path $dll)) { throw "doki_theme.dll not found - build first." }

Copy-Item $dll                                       (Join-Path $stage "plugins")
Copy-Item (Join-Path $repoRoot "definitions\*.json") (Join-Path $stage "doki-theme\definitions")
Copy-Item (Join-Path $repoRoot "assets\stickers\*.png") (Join-Path $stage "doki-theme\assets\stickers")
Copy-Item (Join-Path $repoRoot "ida-plugin.json")    $stage
Copy-Item (Join-Path $repoRoot "INSTALL.md")         $stage

if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "packaged -> $zip"
Get-ChildItem $stage -Recurse -File | ForEach-Object { "  " + $_.FullName.Replace($stage, "doki-theme-ida") }
