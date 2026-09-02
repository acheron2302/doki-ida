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
#   [$env:IDASDK\plugins\doki_theme.dll] (only with -DeployToIdaSdkPlugins)
#
# The theme catalog is embedded into the plugin DLL by CMake (see
# cmake/embed_file.cmake, DOKI_EMBED_CATALOG=ON) and is no longer
# deployed as a separate file.
#
# Sticker and wallpaper assets are downloaded on demand from the
# doki-theme CDN (see src/doki/assets.cpp) and cached under
# $IDAUSR\doki-theme\cache\. No image files are bundled with the plugin.
    [switch]$Build,
    [switch]$DeployToIdaSdkPlugins,
    [switch]$SkipCatalog,
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
  & $cmake --build (Join-Path $repoRoot "build") --config Release
}

# Locate the freshly built DLL (ida-cmake deploys to the SDK bin/plugins).
$dll = Join-Path $env:IDASDK "src\bin\plugins\doki_theme.dll"
if (-not (Test-Path $dll)) { $dll = Join-Path $repoRoot "build\Release\doki_theme.dll" }
Copy-Item $dll (Join-Path $idadir "plugins\doki_theme.dll") -Force
Write-Output "deployed DLL -> $idadir\plugins"

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
        $dst = Join-Path $idadir "plugins" $f.Name
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
  Write-Warning "no libcurl root provided (-CurlDir or $env:DOKI_CURL_DIR). Skipping libcurl deployment."
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

