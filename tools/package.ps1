# Stage a distributable package and zip it into dist/.
#
# Layout (drop the contents into your IDA install dir / $IDAUSR):
#   dist/doki-theme-ida/
#     plugins/doki_theme.dll
#     plugins/libcurl*.dll               (when -CurlDir is supplied)
#     ida-plugin.json
#
# Sticker and wallpaper assets are downloaded on demand from the
# doki-theme CDN (see src/doki/assets.cpp) and cached under
# $IDAUSR\doki-theme\cache\. They are NOT bundled in this package.
#
# Theme metadata is no longer a separate shipped file: the catalog is
# embedded into the plugin DLL at build time by CMake
# (see cmake/embed_file.cmake, DOKI_EMBED_CATALOG=ON).
#
# Flags:
#   -CurlDir <path>     : optional libcurl distribution root/bin to copy
#                         $env:DOKI_CURL_DIR.

param(
    [string]$Version = "0.1.0",
    [string]$DllPath = "",
    [string]$CurlDir = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $repoRoot "dist\doki-theme-ida"
$zip   = Join-Path $repoRoot "dist\doki-theme-ida-$Version.zip"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }

New-Item -ItemType Directory -Force `
  (Join-Path $stage "plugins"),
  (Join-Path $stage "doki-theme") | Out-Null

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
Copy-Item (Join-Path $repoRoot "ida-plugin.json")    $stage
Copy-Item (Join-Path $repoRoot "INSTALL.md")         $stage
# Keep attribution in the package even though bundled images are gone.
if (Test-Path (Join-Path $repoRoot "assets\ATTRIBUTION.md")) {
  Copy-Item (Join-Path $repoRoot "assets\ATTRIBUTION.md") `
            (Join-Path $stage "doki-theme\ATTRIBUTION.md")
}

# Optional libcurl runtime staging. Runtime asset downloads use libcurl
# dynamically instead of Qt Network, so Qt SSL support is not required.
if (-not $CurlDir) { $CurlDir = $env:DOKI_CURL_DIR }
if ($CurlDir -and (Test-Path $CurlDir)) {
  $curlRoots = @($CurlDir, (Join-Path $CurlDir "bin")) | Where-Object { Test-Path $_ }
  $patterns = @(
    "libcurl*.dll", "curl-ca-bundle.crt", "cacert.pem",
    "libssl-3*.dll", "libcrypto-3*.dll", "zlib*.dll", "zstd*.dll",
    "libssh2*.dll", "nghttp2*.dll", "brotli*.dll", "libidn2*.dll",
    "libunistring*.dll"
  )
  $copied = @{}
  foreach ($root in $curlRoots) {
    foreach ($pattern in $patterns) {
      foreach ($f in (Get-ChildItem -LiteralPath $root -File -Filter $pattern -ErrorAction SilentlyContinue)) {
        Copy-Item $f.FullName (Join-Path $stage "plugins" $f.Name) -Force
        $copied[$f.Name] = $true
      }
    }
  }
  if ($copied.Count -gt 0) {
    Write-Output ("staged libcurl runtime ({0} file(s))" -f $copied.Count)
  } else {
    Write-Warning "CurlDir was provided but no libcurl runtime files were found."
  }
} else {
  Write-Warning "no libcurl root provided (-CurlDir or $env:DOKI_CURL_DIR). Skipping libcurl staging."
  Write-Warning "End users need libcurl with HTTPS/TLS support for runtime asset downloads, or can pre-warm the cache with tools\fetch_assets.ps1."
}

if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "packaged -> $zip"
Get-ChildItem $stage -Recurse -File | ForEach-Object { "  " + $_.FullName.Replace($stage, "doki-theme-ida") }
