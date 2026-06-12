# Doki Theme for IDA Pro

Anime character themes — color palettes + character stickers — for IDA Pro,
ported from the [doki-theme](https://github.com/doki-theme) project (VS Code /
JetBrains) to a native **C++ IDA SDK plugin**.

Pick a character from the **Options** menu and IDA is retinted to match: the
disassembly listing, navigation band, selection and chrome take on the
character's palette, with the character's artwork shown behind the listing.

> Status: early (v0.1.0). Bundles 5 themes (Rem, Zero Two — sakura/obsidian,
> Darkness — dark/light) as a working set; more can be added by dropping in
> definitions + assets.

## Features
- Character themes parsed from upstream `*.master.definition.json` files.
- Generates IDA `theme.css` (imports the matching `dark`/`default` base, then
  overrides the disassembly listing, selection, caret and nav band).
- Live **navigation-band** recolor (gradient) via the SDK colorizer — no restart.
- Character **sticker** via CSS `background-image` (`$RELPATH`).
- Menu actions: pick / random / next / toggle sticker / restore default.
- Remembers your character + sticker preference across restarts.
- Restores IDA cleanly on unload (nav colorizer + menu actions removed).

## How it works
| Surface | Mechanism | When it applies |
|---|---|---|
| Disassembly listing, chrome, selection, sticker | generated `theme.css` in `$IDAUSR/themes/doki-<name>/`, activated via the `ThemeName` registry value | next IDA launch / reselect in *Options ▸ Colors* (IDA loads CSS at startup) |
| Navigation band | `set_nav_colorizer` (SDK) | immediately, live |

This split is intentional: IDA applies theme CSS at startup, so chrome/listing
recolor on the next launch, while the nav band is driven live from the SDK.

## Build
Prerequisites: IDA SDK 9.2+ (`IDASDK` env var), CMake 3.27+, Visual Studio 2022
(MSVC), the bundled [`ida-cmake`](https://github.com/allthingsida/ida-cmake)
(ships inside the official HexRaysSA/ida-sdk under `$IDASDK/ida-cmake`).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# deploy for local testing (DLL -> IDADIR\plugins, assets -> $IDAUSR\doki-theme)
tools\deploy.ps1
# build a distributable zip in dist\
cmake --build build --target package        # or: tools\package.ps1
```

See [INSTALL.md](INSTALL.md) for end-user install steps.

## Adding more characters
1. Add the `*.master.definition.json` to `definitions/`.
2. Add its sticker PNG to `assets/stickers/` (see `tools/fetch_assets.ps1`).
3. (Optional) Add a full-listing wallpaper PNG to `assets/wallpapers/` and
   declare it in the definition under a top-level `"background"` block:
   ```json
   "background": { "name": "my_char.png", "anchor": "center", "opacity": 100 }
   ```
   If `background` is omitted the installer falls back to looking for
   `assets/wallpapers/<sticker name>`.
4. Re-run `tools\deploy.ps1`. The new character appears in the picker.

## Cross-platform notes
The engine is plain C++ + IDA SDK (no Qt linkage), so it ports cleanly:
- **Output name:** `doki_theme.dll` (Windows) / `.so` (Linux) / `.dylib` (macOS)
  — handled by `ida_add_plugin`.
- **`$IDAUSR` default:** `%APPDATA%\Hex-Rays\IDA Pro` (Windows),
  `~/.idapro` (Linux/macOS). All paths resolve via `get_user_idadir()`.
- **Theme activation:** uses the `ThemeName` registry/config value through the
  SDK registry API, which is abstracted per-platform by IDA.
- Path separator handling is centralized in `src/doki/paths.cpp`.

Only Windows is built/tested so far; Linux/macOS should build with the same
CMake against their SDKs.

## Attribution
Theme palettes and character art come from the **doki-theme** project by
Unthrottled and contributors — see [assets/ATTRIBUTION.md](assets/ATTRIBUTION.md).
Character artwork belongs to the respective studios/artists.

## License
Plugin code: MIT (see LICENSE). Bundled doki assets retain their upstream terms.
