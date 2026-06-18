# Stage a distributable package and zip it into dist/.
#
# Layout (drop the contents into your IDA install dir / $IDAUSR):
#   dist/doki-theme-ida/
#     plugins/doki_theme.dll
#     doki-theme/definitions/*.json
#     ida-plugin.json
#     INSTALL.md
#
# Sticker and wallpaper assets are downloaded on demand from the
# doki-theme CDN (see src/doki/assets.cpp) and cached under
# $IDAUSR\doki-theme\cache\. They are NOT bundled in this package.
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
  (Join-Path $stage "doki-theme\definitions") | Out-Null

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
Copy-Item (Join-Path $repoRoot "ida-plugin.json")    $stage
Copy-Item (Join-Path $repoRoot "INSTALL.md")         $stage
# Keep attribution in the package even though bundled images are gone.
if (Test-Path (Join-Path $repoRoot "assets\ATTRIBUTION.md")) {
  Copy-Item (Join-Path $repoRoot "assets\ATTRIBUTION.md") `
            (Join-Path $stage "doki-theme\ATTRIBUTION.md")
}

if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "packaged -> $zip"
Get-ChildItem $stage -Recurse -File | ForEach-Object { "  " + $_.FullName.Replace($stage, "doki-theme-ida") }
