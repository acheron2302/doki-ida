# Generate the IDA theme catalog from the snapshotted upstream
# doki-master-theme definitions.
#
# Input  : third_party/doki-master-theme/definitions/**/*.master.definition.json
# Output : generated/theme_catalog.json
#
# The catalog is the single runtime source of truth: the plugin loads
# the catalog instead of scanning a per-theme definitions/ folder at
# startup. See .kilo/plans/build-themes-from-upstream-install.md.
#
# Sticker / wallpaper CDN paths follow the upstream convention from
# doki-build-source's resolveStickerPath():
#   - sticker : stickers/vscode/<dir-of-def-json>/<stickers.default.name>
#               e.g.  reZero/rem/rem.master.definition.json + rem.png
#                  => stickers/vscode/reZero/rem/rem.png
#               e.g.  franxx/zeroTwo/obsidian/zero.two.obsidian.master.definition.json
#                  + zero_two_obsidian.png
#                  => stickers/vscode/franxx/zeroTwo/obsidian/zero_two_obsidian.png
#   - wallpaper: backgrounds/wallpapers/transparent/<stickers.default.name>
#               (the transparent variant lets the palette show through).
#
# Determinism: stable sort by (group, displayName, name, id); stable
# key order in every emitted object; no wall-clock timestamps unless
# -IncludeTimestamps is passed (useful for local debugging only).
#
# Exit codes: 0 on success, 1 if any theme is rejected (see skips[]).

param(
    [string]$DefinitionsDir = (Join-Path (Split-Path -Parent $PSScriptRoot) "third_party\doki-master-theme\definitions"),
    [string]$OutputPath     = (Join-Path (Split-Path -Parent $PSScriptRoot) "generated\theme_catalog.json"),
    [string]$UpstreamRef    = "",
    [switch]$IncludeTimestamps
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

# Locate the UPSTREAM_PIN.json (written by fetch_upstream.ps1) so the
# generated catalog can self-document its upstream ref.
if (-not $UpstreamRef) {
    $pinPath = Join-Path (Split-Path -Parent $DefinitionsDir) "UPSTREAM_PIN.json"
    if (Test-Path $pinPath) {
        $pin = Get-Content -LiteralPath $pinPath -Raw | ConvertFrom-Json
        $UpstreamRef = $pin.ref
    }
}

if (-not (Test-Path $DefinitionsDir)) {
    throw "definitions dir not found: $DefinitionsDir (run tools\fetch_upstream.ps1 first)"
}

# Emit a deterministic object: PSCustomObject -> ordered hashtable -> ordered PSCustomObject
# so that field order is stable across runs (which is what makes the
# output byte-stable and CI diffs reviewable).
function OrderedObject {
    param([hashtable]$Table)
    $ordered = [ordered]@{}
    foreach ($k in $Table.Keys) { $ordered[$k] = $Table[$k] }
    return [pscustomobject]$ordered
}

# Computes the upstream sticker path from a definition's relative path
# and its stickers.default.name. Mirrors resolveStickerPath() from
# doki-build-source: parent dir of the JSON + sticker file name, with
# the leading "definitions/" prefix stripped.
function Get-StickerPath {
    param([string]$RelJsonPath, [string]$StickerName)
    # $RelJsonPath is e.g. "reZero\rem\rem.master.definition.json" on
    # Windows. Normalize to forward slashes and strip the basename.
    $norm = ($RelJsonPath -replace "\\", "/")
    $idx = $norm.LastIndexOf("/")
    if ($idx -lt 0) { $dir = "" } else { $dir = $norm.Substring(0, $idx) }
    if ($dir.Length -gt 0) {
        return "stickers/vscode/$dir/$StickerName"
    }
    return "stickers/vscode/$StickerName"
}

# Stable sort: (group, displayName, name, id), all ordinal case-insensitive.
function Compare-Theme {
    param($a, $b)
    foreach ($k in "group","displayName","name","id") {
        $x = [string]$a.$k
        $y = [string]$b.$k
        $c = [string]::Compare($x, $y, $true)  # case-insensitive ordinal
        if ($c -ne 0) { return $c }
    }
    return 0
}

$entries = @()
$skipped = @()
$defsRoot = (Resolve-Path -LiteralPath $DefinitionsDir).Path
$files = Get-ChildItem -LiteralPath $defsRoot -Recurse -File -Filter "*.master.definition.json" | Sort-Object FullName

foreach ($f in $files) {
    $rel = $f.FullName.Substring($defsRoot.Length).TrimStart("\","/")

    $raw = Get-Content -LiteralPath $f.FullName -Raw -ErrorAction Stop
    $j = $null
    try { $j = $raw | ConvertFrom-Json -ErrorAction Stop }
    catch {
        $skipped += [pscustomobject]@{ path = $rel; reason = "json parse: $($_.Exception.Message)" }
        continue
    }

    # Required fields. displayName falls back to name.
    $id    = [string]$j.id
    $name  = [string]$j.name
    $disp  = if ($j.PSObject.Properties.Match('displayName').Count) { [string]$j.displayName } else { $name }
    $group = if ($j.PSObject.Properties.Match('group').Count) { [string]$j.group } else { "" }
    $auth  = if ($j.PSObject.Properties.Match('author').Count) { [string]$j.author } else { "" }
    $dark  = if ($j.PSObject.Properties.Match('dark').Count) { [bool]$j.dark } else { $true }
    $prod  = if ($j.PSObject.Properties.Match('product').Count) { [string]$j.product } else { "community" }
    $cid   = if ($j.PSObject.Properties.Match('characterId').Count) { [string]$j.characterId } else { "" }

    if (-not $id)    { $skipped += [pscustomobject]@{ path=$rel; reason="missing id" }; continue }
    if (-not $name)  { $skipped += [pscustomobject]@{ path=$rel; reason="missing name" }; continue }

    # stickers.default is required (every upstream entry has it; we
    # still validate rather than assume).
    $stickerName  = $null
    $stickerAnch  = "right"
    $stickerOpa   = $null
    $secondaryObj = $null
    if ($j.PSObject.Properties.Match('stickers').Count -and $j.stickers) {
        $st = $j.stickers
        if ($st.PSObject.Properties.Match('default').Count -and $st.default) {
            $d = $st.default
            if ($d.PSObject.Properties.Match('name').Count)   { $stickerName = [string]$d.name }
            if ($d.PSObject.Properties.Match('anchor').Count) { $stickerAnch = [string]$d.anchor }
            if ($d.PSObject.Properties.Match('opacity').Count){ $stickerOpa  = $d.opacity }
        }
        if ($st.PSObject.Properties.Match('secondary').Count -and $st.secondary) {
            $secondaryObj = $st.secondary
        }
    }
    if (-not $stickerName) {
        $skipped += [pscustomobject]@{ path=$rel; reason="missing stickers.default.name" }
        continue
    }

    # baseBackground is the only color the runtime strictly needs (see
    # load_definition in src/doki/theme.cpp). Surface other colors for
    # future IDA surfaces but keep the parser contract simple.
    if (-not $j.PSObject.Properties.Match('colors').Count -or -not $j.colors.baseBackground) {
        $skipped += [pscustomobject]@{ path=$rel; reason="missing colors.baseBackground" }
        continue
    }

    # Build a deterministic colors object (sorted keys, normalized hex
    # to lowercase). Values that are not strings are dropped.
    $colorsOut = [ordered]@{}
    foreach ($k in ($j.colors.PSObject.Properties | Sort-Object Name)) {
        $v = $k.Value
        if ($v -is [string]) {
            $vs = [string]$v
            if ($vs.Length -gt 0 -and $vs[0] -eq '#') { $vs = $vs.ToLower() }
            $colorsOut[$k.Name] = $vs
        }
    }
    $colorsTable = [ordered]@{}
    foreach ($k in $colorsOut.Keys) { $colorsTable[$k] = $colorsOut[$k] }
    $colorsObj = [pscustomobject]$colorsTable

    # Overrides.editorScheme.colors: same treatment.
    $overridesObj = $null
    $hasOverrides = $j.PSObject.Properties.Match('overrides').Count -and $j.overrides
    $hasEdScheme  = $hasOverrides -and $j.overrides.PSObject.Properties.Match('editorScheme').Count -and $j.overrides.editorScheme
    $hasEdColors  = $hasEdScheme -and $j.overrides.editorScheme.PSObject.Properties.Match('colors').Count -and $j.overrides.editorScheme.colors
    if ($hasEdColors) {
        $os = [ordered]@{}
        foreach ($k in ($j.overrides.editorScheme.colors.PSObject.Properties | Sort-Object Name)) {
            $v = $k.Value
            if ($v -is [string]) {
                $vs = [string]$v
                if ($vs.Length -gt 0 -and $vs[0] -eq '#') { $vs = $vs.ToLower() }
                $os[$k.Name] = $vs
            }
        }
        $ostable = [ordered]@{}
        foreach ($k in $os.Keys) { $ostable[$k] = $os[$k] }
        $overridesObj = [pscustomobject]@{ editorScheme = [pscustomobject]@{ colors = [pscustomobject]$ostable } }
    }

    $stickerPath = Get-StickerPath -RelJsonPath $rel -StickerName $stickerName
    $wallpaperPath = "backgrounds/wallpapers/transparent/$stickerName"

    # Stickers object: default + optional secondary.
    # NOTE: the IDA overlay pins the sticker to the bottom-right corner of the
    # active view regardless of what the upstream definition says (the
    # upstream JSON still carries per-character anchors from its CSS-only
    # days, e.g. "center" for Echidna). We normalize every emitted sticker
    # anchor to "bottom" so the catalog is self-consistent with that policy;
    # the runtime also enforces it as a safety net (see kOverlayStickerAnchor
    # in src/doki/apply.cpp).
    $stickerTbl = [ordered]@{}
    $defaultTbl = [ordered]@{
        name        = $stickerName
        anchor      = "bottom"
        remotePath  = $stickerPath
    }
    if ($null -ne $stickerOpa) { $defaultTbl["opacity"] = $stickerOpa }
    $stickerTbl["default"] = [pscustomobject]$defaultTbl

    if ($secondaryObj) {
        $secName = $null
        $secAnch = $null
        $secOpa  = $null
        if ($secondaryObj.PSObject.Properties.Match('name').Count)    { $secName = [string]$secondaryObj.name }
        if ($secondaryObj.PSObject.Properties.Match('anchor').Count)  { $secAnch = [string]$secondaryObj.anchor }
        if ($secondaryObj.PSObject.Properties.Match('opacity').Count) { $secOpa  = $secondaryObj.opacity }
        if ($secName) {
            $secStickerPath = Get-StickerPath -RelJsonPath $rel -StickerName $secName
            $secTbl = [ordered]@{
                name        = $secName
                anchor      = "bottom"
                remotePath  = $secStickerPath
            }
            if ($null -ne $secOpa) { $secTbl["opacity"] = $secOpa }
            $stickerTbl["secondary"] = [pscustomobject]$secTbl
        }
    }

    $bgTbl = [ordered]@{
        name        = $stickerName
        anchor      = "center"
        remotePath  = $wallpaperPath
    }

    $entry = [pscustomobject]([ordered]@{
        id           = $id
        characterId  = $cid
        name         = $name
        displayName  = $disp
        group        = $group
        author       = $auth
        dark         = $dark
        product      = $prod
        colors       = $colorsObj
        overrides    = $overridesObj
        stickers     = [pscustomobject]$stickerTbl
        background   = [pscustomobject]$bgTbl
        sourcePath   = "definitions/$($rel -replace '\\','/')"
    })
    $entries += ,$entry
}

# Sort deterministically.
$sorted = ($entries | Sort-Object -Property `
    @{Expression={$_.group}; Ascending=$true},
    @{Expression={$_.displayName}; Ascending=$true},
    @{Expression={$_.name}; Ascending=$true},
    @{Expression={$_.id}; Ascending=$true})

# Source / provenance block.
$sourceTbl = [ordered]@{
    schemaVersion = 1
    generator     = "tools/generate_theme_catalog.ps1"
}
if ($UpstreamRef) { $sourceTbl["upstreamRef"] = $UpstreamRef }
if ($IncludeTimestamps) {
    $sourceTbl["generatedAt"] = (Get-Date).ToUniversalTime().ToString("o")
}

$catalog = [pscustomobject]([ordered]@{
    source  = [pscustomobject]$sourceTbl
    themes  = $sorted
})

# Emitted metadata (skipped themes) is reported via stderr / console.
# The catalog itself contains only the usable themes.
$outDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }
# Use a 2-space indent for stable, human-readable diffs.
$json = $catalog | ConvertTo-Json -Depth 16
Set-Content -LiteralPath $OutputPath -Value $json -Encoding utf8

Write-Output "catalog -> $OutputPath"
Write-Output ("  themes : {0}" -f $sorted.Count)
Write-Output ("  skipped: {0}" -f $skipped.Count)
foreach ($s in $skipped) {
    Write-Warning ("  skip '{0}': {1}" -f $s.path, $s.reason)
}
if ($skipped.Count -gt 0) { exit 1 }
