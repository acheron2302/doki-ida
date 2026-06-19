# Snapshot the upstream doki-theme/doki-master-theme "definitions" tree
# into third_party/doki-master-theme/definitions/ so the catalog generator
# can run reproducibly without a Node/npm toolchain or live network access
# at build time.
#
# Why a snapshot instead of npm:
#   * Node/npm are not part of the documented dev environment; PowerShell
#     already is (deploy.ps1, package.ps1, fetch_assets.ps1, CI).
#   * The plan's "pinned github: doki-master-theme" dependency is not on
#     the npm registry, so installation reliability is unverified.
#   * A pinned commit produces byte-identical definitions, which the
#     generator can hash to detect drift.
#
# Implementation note: uses the GitHub codeload tarball
# (https://codeload.github.com/.../tar.gz/<sha>) which is a single
# request with no per-IP rate limit, rather than the Contents / Git
# Trees APIs which we hit a 60-req/hr anonymous rate limit on.
#
# Usage:
#   tools\fetch_upstream.ps1                 # fetch pinned ref
#   tools\fetch_upstream.ps1 -Ref <sha>      # fetch a specific commit
#   tools\fetch_upstream.ps1 -NoFetch        # regenerate manifest from
#                                            # already-downloaded tree
param(
    [string]$Ref = "4467240b45837ff473903d572790112fc2caa8ca",
    [switch]$NoFetch
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$dstRoot  = Join-Path $repoRoot "third_party\doki-master-theme"
$defsRoot = Join-Path $dstRoot  "definitions"
$manifest = Join-Path $dstRoot  "UPSTREAM_PIN.json"
$tarball  = Join-Path ([IO.Path]::GetTempPath()) "doki-master-theme-$Ref.tar.gz"

if (-not $NoFetch) {
    Write-Output "fetching doki-master-theme @ $Ref (codeload tarball)..."
    $url = "https://codeload.github.com/doki-theme/doki-master-theme/tar.gz/$Ref"
    Invoke-WebRequest -Uri $url -OutFile $tarball -UseBasicParsing -Headers @{
        "User-Agent" = "doki-theme-ida-fetch"
    }
    $size = (Get-Item $tarball).Length
    if ($size -lt 1024) {
        Remove-Item $tarball -Force
        throw "tarball suspiciously small ($size bytes); refusing to extract"
    }

    if (Test-Path $defsRoot) { Remove-Item $defsRoot -Recurse -Force }
    New-Item -ItemType Directory -Force $dstRoot | Out-Null

    # Extract only the "definitions/" subtree.
    # tar --strip-components=1 -xf $tarball --include='*/definitions/*' -C $dstRoot
    $include = "*/definitions/*"
    & tar --strip-components=1 -xf $tarball "--include=$include" -C $dstRoot
    if ($LASTEXITCODE -ne 0) {
        Remove-Item $tarball -Force
        throw "tar extraction failed (exit $LASTEXITCODE)"
    }
    Remove-Item $tarball -Force

    if (-not (Test-Path $defsRoot)) {
        throw "extracted tree is missing definitions/"
    }
    $count = (Get-ChildItem -LiteralPath $defsRoot -Recurse -File).Count
    Write-Output "extracted $count file(s) to $defsRoot"
}

# Always (re)write the manifest. Counts files, hashes the tree.
$localFiles = Get-ChildItem -LiteralPath $defsRoot -Recurse -File |
              Sort-Object FullName

$sha = [System.Security.Cryptography.SHA256]::Create()
foreach ($f in $localFiles) {
    $bytes = [IO.File]::ReadAllBytes($f.FullName)
    $sha.TransformBlock($bytes, 0, $bytes.Length, $null, 0) | Out-Null
}
$sha.TransformFinalBlock([byte[]]@(), 0, 0) | Out-Null
$treeHash = ([BitConverter]::ToString($sha.Hash) -replace "-", "").ToLower()

$manifestObj = [pscustomobject]@{
    repo        = "doki-theme/doki-master-theme"
    ref         = $Ref
    fetchedAt   = (Get-Date).ToUniversalTime().ToString("o")
    fileCount   = $localFiles.Count
    treeSha256  = $treeHash
}
$manifestObj | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifest -Encoding utf8
Write-Output "manifest -> $manifest"
Write-Output "  ref=$Ref  files=$($localFiles.Count)  treeSha256=$treeHash"
