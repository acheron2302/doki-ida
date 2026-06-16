# Stage a distributable package and zip it into dist/.
#
# Layout (drop the contents into your IDA install dir / $IDAUSR):
#   dist/doki-theme-ida/
#     plugins/doki_theme.dll
#     doki-theme/definitions/*.json
#     doki-theme/assets/stickers/*.png
#     doki-theme/assets/wallpapers/*.png
#     ida-plugin.json
#     INSTALL.md
param(
    [string]$Version = "0.1.0",
    [string]$DllPath = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $repoRoot "dist\doki-theme-ida"
$zip   = Join-Path $repoRoot "dist\doki-theme-ida-$Version.zip"

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force `
  (Join-Path $stage "plugins"),
  (Join-Path $stage "doki-theme\definitions"),
  (Join-Path $stage "doki-theme\assets\stickers"),
  (Join-Path $stage "doki-theme\assets\wallpapers") | Out-Null

# Resolve the DLL: explicit -DllPath wins, then well-known ida-cmake output
# locations in priority order (mirrors what CI verifies).
$dll = ""
if ($DllPath) {
    if (-not (Test-Path $DllPath)) { throw "-DllPath does not exist: $DllPath" }
    $dll = $DllPath
} else {
    $candidates = @(
        (Join-Path $env:IDASDK "src\bin\plugins\doki_theme.dll"),
        (Join-Path $repoRoot "build\output\plugins\doki_theme.dll"),
        (Join-Path $repoRoot "build\Release\doki_theme.dll"),
        (Join-Path $repoRoot "build\bin\plugins\doki_theme.dll")
    )
    $dll = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
}
if (-not $dll) { throw "doki_theme.dll not found - build first." }

Copy-Item $dll                                       (Join-Path $stage "plugins")
Copy-Item (Join-Path $repoRoot "definitions\*.json") (Join-Path $stage "doki-theme\definitions")
Copy-Item (Join-Path $repoRoot "assets\stickers\*.png") (Join-Path $stage "doki-theme\assets\stickers")
if (Test-Path (Join-Path $repoRoot "assets\wallpapers")) {
  Copy-Item (Join-Path $repoRoot "assets\wallpapers\*.png") (Join-Path $stage "doki-theme\assets\wallpapers")
}
Copy-Item (Join-Path $repoRoot "ida-plugin.json")    $stage
Copy-Item (Join-Path $repoRoot "INSTALL.md")         $stage

if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "packaged -> $zip"
Get-ChildItem $stage -Recurse -File | ForEach-Object { "  " + $_.FullName.Replace($stage, "doki-theme-ida") }
