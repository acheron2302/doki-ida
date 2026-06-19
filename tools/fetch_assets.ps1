# Developer utility: pre-warm the local cache from the deployed catalog.
#
# The runtime (src/doki/assets.cpp) downloads on demand from the
# doki-theme CDN. This script is provided for offline development /
# air-gapped testing: it walks generated/theme_catalog.json (or the
# installed copy under $IDAUSR\doki-theme\) and downloads every
# unique sticker and transparent wallpaper PNG into the same
# authoritative cache layout the runtime uses.
#
# Usage:
#   tools\fetch_assets.ps1                       # cache under $IDAUSR\doki-theme\cache
#   tools\fetch_assets.ps1 -OutDir <path>        # cache under <path>\cache
#   tools\fetch_assets.ps1 -CatalogPath <path>   # use a specific catalog
#   tools\fetch_assets.ps1 -GroupLike "Re Zero"  # only themes whose group matches
#   tools\fetch_assets.ps1 -NameLike "Echidna"   # only themes whose name matches
#   tools\fetch_assets.ps1 -ThemeId "<id>"       # only the theme with this id
#   tools\fetch_assets.ps1 -ListOnly             # print what would be fetched, do not download
param(
    [string]$OutDir      = "",
    [string]$CatalogPath = "",
    [string]$GroupLike   = "",
    [string]$NameLike    = "",
    [string]$ThemeId     = "",
    [switch]$ListOnly
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $OutDir) {
    $idausr = $env:IDAUSR
    if (-not $idausr) { $idausr = Join-Path $env:APPDATA "Hex-Rays\IDA Pro" }
    $OutDir = Join-Path $idausr "doki-theme"
}

# Prefer the catalog in the deploy dir; fall back to generated/.
if (-not $CatalogPath) {
    $candidates = @(
        (Join-Path $OutDir "theme_catalog.json"),
        (Join-Path $repoRoot "generated\theme_catalog.json")
    )
    $CatalogPath = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $CatalogPath -or -not (Test-Path $CatalogPath)) {
    throw "no theme_catalog.json found; run tools\generate_theme_catalog.ps1 or deploy first"
}

$stickersDir = Join-Path $OutDir "cache\stickers"
$wallDir     = Join-Path $OutDir "cache\wallpapers"
New-Item -ItemType Directory -Force $stickersDir, $wallDir | Out-Null

$cdn = "https://doki.assets.unthrottled.io"
$cat = Get-Content -LiteralPath $CatalogPath -Raw | ConvertFrom-Json

# Pre-compute the per-theme "display key" used for filters: the most
# common pattern is a wildcard on a character name (e.g. "Echidna").
# We match against id, name, displayName, and group in case-insensitive
# wildcard form so -NameLike is forgiving about which field the user
# remembers. -ThemeId is a strict equality match (ids are GUIDs).
function Test-Filter {
    param($t)
    if ($ThemeId)  { if ($t.id -ne $ThemeId) { return $false } }
    if ($GroupLike) {
        $g = [string]$t.group
        if (-not ($g -like "*$GroupLike*")) { return $false }
    }
    if ($NameLike) {
        $matched = $false
        foreach ($field in "name","displayName","id") {
            $v = [string]$t.$field
            if ($v -like "*$NameLike*") { $matched = $true; break }
        }
        if (-not $matched) { return $false }
    }
    return $true
}

# Collect every (kind, fileName, url) tuple from the catalog.
$jobs = New-Object System.Collections.Generic.List[object]
$selectedThemes = New-Object System.Collections.Generic.List[object]
foreach ($t in $cat.themes) {
    if (-not (Test-Filter $t)) { continue }
    $selectedThemes.Add($t) | Out-Null
    if ($t.stickers -and $t.stickers.default -and $t.stickers.default.name -and $t.stickers.default.remotePath) {
        $jobs.Add([pscustomobject]@{
            kind  = "sticker"
            name  = [string]$t.stickers.default.name
            url   = "$cdn/$($t.stickers.default.remotePath)"
            theme = [string]$t.displayName
        })
    }
    if ($t.stickers -and $t.stickers.secondary -and $t.stickers.secondary.name -and $t.stickers.secondary.remotePath) {
        $jobs.Add([pscustomobject]@{
            kind  = "sticker"
            name  = [string]$t.stickers.secondary.name
            url   = "$cdn/$($t.stickers.secondary.remotePath)"
            theme = [string]$t.displayName
        })
    }
    if ($t.background -and $t.background.name -and $t.background.remotePath) {
        $jobs.Add([pscustomobject]@{
            kind  = "wallpaper"
            name  = [string]$t.background.name
            url   = "$cdn/$($t.background.remotePath)"
            theme = [string]$t.displayName
        })
    }
}

# Print the selection so the user can confirm before any download runs.
Write-Output ("catalog: {0}" -f $CatalogPath)
Write-Output ("filter : GroupLike='{0}'  NameLike='{1}'  ThemeId='{2}'" -f $GroupLike, $NameLike, $ThemeId)
Write-Output ("selected {0} theme(s):" -f $selectedThemes.Count)
foreach ($t in $selectedThemes) {
    Write-Output ("  - {0,-40} [{1}]  id={2}" -f $t.displayName, $t.group, $t.id)
}
Write-Output ("{0} asset job(s) queued" -f $jobs.Count)
foreach ($j in $jobs) {
    $dir = if ($j.kind -eq "sticker") { $stickersDir } else { $wallDir }
    $dst = Join-Path $dir $j.name
    Write-Output ("  {0,-9} {1}  -> {2}" -f $j.kind, $j.name, $dst)
    Write-Output ("             url: {0}" -f $j.url)
}
if ($ListOnly) {
    Write-Output "ListOnly set; not downloading."
    return
}
if ($jobs.Count -eq 0) {
    Write-Warning "no assets matched the filter; nothing to download."
    return
}

# Deduplicate (fileName, kind) so we only fetch each PNG once.
$seen = @{}
$fetched = 0
$skipped = 0
foreach ($j in $jobs) {
    $key = "$($j.kind)/$($j.name)"
    if ($seen.ContainsKey($key)) { $skipped++; continue }
    $seen[$key] = $true

    $dir = if ($j.kind -eq "sticker") { $stickersDir } else { $wallDir }
    $dst = Join-Path $dir $j.name
    if (Test-Path $dst) { $skipped++; continue }

    try {
        Invoke-WebRequest -Uri $j.url -OutFile $dst -UseBasicParsing
        $fetched++
        Write-Output ("fetched {0,-9} {1}" -f $j.kind, $j.name)
    }
    catch {
        Write-Warning ("failed  {0,-9} {1}: {2}" -f $j.kind, $j.name, $_.Exception.Message)
    }
}
Write-Output ("done -> {0}\cache  (fetched={1}, skipped={2}, total={3})" -f $OutDir, $fetched, $skipped, $seen.Count)
