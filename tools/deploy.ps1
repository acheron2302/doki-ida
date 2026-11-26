# Build (optional) + deploy the plugin and its assets for local testing.
#
#   tools\deploy.ps1                          # deploy DLL (+ libcurl if -CurlDir)
#   tools\deploy.ps1 -Build                   # cmake --build first
#   tools\deploy.ps1 -DeployToIdaSdkPlugins   # also copy DLL to $env:IDASDK\plugins
#   tools\deploy.ps1 -CurlDir <path>          # optional libcurl distribution root/bin
#                                             # to copy libcurl + common TLS deps from.
#                                             # Defaults to $env:DOKI_CURL_DIR.
#
# Stages:
#   <IDADIR>\plugins\doki_theme.dll
#   <IDADIR>\plugins\libcurl*.dll              (if -CurlDir / $DOKI_CURL_DIR is provided)
#   $IDAUSR\doki-theme\theme_catalog.json      (fallback for embed-off builds)
#   [$env:IDASDK\plugins\doki_theme.dll] (only with -DeployToIdaSdkPlugins)
#
# The theme catalog is embedded into the plugin DLL by CMake (see
# cmake/embed_file.cmake, DOKI_EMBED_CATALOG=ON) and is normally not
# needed on disk. For local DOKI_EMBED_CATALOG=OFF builds this script
# also stages generated/theme_catalog.json under $IDAUSR\doki-theme\;
# the embedded build ignores that file.
#
# Sticker and wallpaper assets are downloaded on demand from the
# doki-theme CDN (see src/doki/assets.cpp) and cached under
# $IDAUSR\doki-theme\cache\. No image files are bundled with the plugin.
param(
    [switch]$Build,
    [switch]$DeployToIdaSdkPlugins,
    [string]$CurlDir = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$idadir = $env:IDADIR
if (-not $idadir) { $idadir = "D:\tools\IDA 9.4" }
$idausr = $env:IDAUSR
if (-not $idausr) { $idausr = Join-Path $env:APPDATA "Hex-Rays\IDA Pro" }

if ($Build) {
  $cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  if (-not (Test-Path $cmake)) {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
  }
  & $cmake --build (Join-Path $repoRoot "build") --config Release
  if ($LASTEXITCODE -ne 0) { throw "cmake build failed (exit $LASTEXITCODE)" }
}

# Locate the freshly built DLL (ida-cmake deploys to the SDK bin/plugins).
$dll = ""
if ($env:IDASDK) {
  $sdkDll = Join-Path $env:IDASDK "src\bin\plugins\doki_theme.dll"
  if (Test-Path $sdkDll) { $dll = $sdkDll }
}
if (-not $dll) {
  $buildDll = Join-Path $repoRoot "build\Release\doki_theme.dll"
  if (Test-Path $buildDll) { $dll = $buildDll }
}
if (-not $dll) {
  throw "doki_theme.dll not found (build first, or pass -Build)."
}
$pluginsDir = Join-Path $idadir "plugins"
if (-not (Test-Path $pluginsDir)) { throw "IDA plugins directory not found: $pluginsDir" }
Copy-Item $dll (Join-Path $pluginsDir "doki_theme.dll") -Force
Write-Output "deployed DLL -> $pluginsDir"

# Optional on-disk catalog for local DOKI_EMBED_CATALOG=OFF builds.
$catalog = Join-Path $repoRoot "generated\theme_catalog.json"
if (Test-Path $catalog) {
  try {
    $catalogDir = Join-Path $idausr "doki-theme"
    New-Item -ItemType Directory -Force $catalogDir | Out-Null
    Copy-Item $catalog (Join-Path $catalogDir "theme_catalog.json") -Force
    Write-Output "deployed catalog fallback -> $catalogDir\theme_catalog.json"
  } catch {
    Write-Warning "could not stage catalog fallback: $($_.Exception.Message)"
  }
}

# ---------------------------------------------------------------------------
# Optional libcurl runtime deployment. Runtime asset downloads use libcurl
# dynamically instead of Qt Network, so Qt SSL support is not required. If
# libcurl is missing the plugin degrades gracefully and tells the user to
# pre-warm the cache with tools\fetch_assets.ps1.
# ---------------------------------------------------------------------------
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
        $dst = Join-Path (Join-Path $idadir "plugins") $f.Name
        Copy-Item $f.FullName $dst -Force
        $copied[$f.Name] = $true
      }
    }
  }
  if ($copied.Count -gt 0) {
    Write-Output ("deployed libcurl runtime ({0} file(s)) -> {1}" -f $copied.Count, (Join-Path $idadir "plugins"))
  } else {
    Write-Warning "CurlDir was provided but no libcurl runtime files were found."
  }
} else {
  Write-Warning 'no libcurl root provided (-CurlDir or $env:DOKI_CURL_DIR). Skipping libcurl deployment.'
  Write-Warning "Runtime asset downloads require libcurl with HTTPS/TLS support; otherwise use tools\fetch_assets.ps1 to pre-warm the cache."
}

if ($DeployToIdaSdkPlugins) {
  if (-not $env:IDASDK) { throw "IDASDK env var is required for -DeployToIdaSdkPlugins" }
  $sdkPlugins = Join-Path $env:IDASDK "plugins"
  New-Item -ItemType Directory -Force $sdkPlugins | Out-Null
  Copy-Item $dll (Join-Path $sdkPlugins "doki_theme.dll") -Force
  Write-Output "deployed DLL -> $sdkPlugins"
}

# The theme catalog is embedded into the plugin DLL by CMake at build
# time (see cmake/embed_file.cmake, DOKI_EMBED_CATALOG=ON). It is no
# longer deployed to $IDAUSR\doki-theme\.

