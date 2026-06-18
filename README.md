# Doki Theme for IDA Pro

Anime character themes — color palettes + character stickers — for IDA Pro,
ported from the [doki-theme](https://github.com/doki-theme) project (VS Code /
JetBrains) to a native **C++ IDA SDK plugin**.

Pick a character from the **Options** menu and IDA is retinted to match: the
disassembly listing, navigation band, selection and chrome take on the
character's palette, with the character's artwork shown behind the listing.

> Status: early (v0.1.0). Bundles 5 themes (Rem, Zero Two — sakura/obsidian,
> Darkness — dark/light) as a working set; more can be added by dropping in
> additional definitions and extending the asset lookup table.

## Screenshots

Darkness (dark variant) — disassembly listing, navigation band, chrome and
sticker all recolored from the character's palette:

![Doki Theme — Darkness (dark)](docs/screenshots/doki-theme-darkness-dark.png)

## Features
- Character themes parsed from upstream `*.master.definition.json` files.
- Generates IDA `theme.css` (imports the matching `dark`/`default` base, then
  overrides the disassembly listing, selection, caret and nav band).
- Live **navigation-band** recolor (gradient) via the SDK colorizer — no restart.
- Character **sticker** rendered by a transparent Qt overlay on top of the
  active viewer (no mouse/focus interference).
- Full-listing **wallpaper** painted on `CustomIDAMemo` via CSS
  `background-image` (`$RELPATH`); transparency comes from the per-pixel
  alpha baked into the upstream Doki PNG.
- **On-demand asset download** from the authoritative doki-theme CDN
  (`https://doki.assets.unthrottled.io`). The first apply fetches each
  theme's sticker and transparent wallpaper; later applies (including
  offline sessions) read from the local cache under
  `$IDAUSR\doki-theme\cache\`.
- Menu actions: pick / random / next / toggle sticker / toggle wallpaper /
  restore default.
- Remembers your character + sticker / wallpaper preferences across restarts.
- Restores IDA cleanly on unload (nav colorizer + menu actions removed).

## How it works
| Surface | Mechanism | When it applies |
|---|---|---|
| Sticker and wallpaper PNGs | downloaded on first use, cached under `$IDAUSR/doki-theme/cache/{stickers,wallpapers}/` | transparent, no UI prompts; offline after first use |
| Disassembly listing, chrome, selection | generated `theme.css` in `$IDAUSR/themes/doki-<name>/`, activated via the `ThemeName` registry value | next IDA launch / reselect in *Options ▸ Colors* (IDA loads CSS at startup) |
| Sticker on the active view | transparent Qt overlay (input pass-through) | immediately, live |
| Navigation band | `set_nav_colorizer` (SDK) | immediately, live |

The CSS uses the per-pixel alpha of the upstream Doki PNG; no CSS opacity,
no Qt wallpaper layer, no runtime transparency controls.

## Build
Prerequisites: IDA SDK 9.2+ (`IDASDK` env var), CMake 3.27+, Visual Studio 2022
(MSVC), the bundled [`ida-cmake`](https://github.com/allthingsida/ida-cmake)
(ships inside the official HexRaysSA/ida-sdk under `$IDASDK/ida-cmake`).
The plugin also links against `Qt6::Network` (the developer's Qt6 install,
pointed to via `DOKI_QT_DIR` or `CMAKE_PREFIX_PATH`).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# deploy for local testing (DLL -> IDADIR\plugins, definitions -> $IDAUSR\doki-theme)
tools\deploy.ps1
# build a distributable zip in dist\
cmake --build build --target package        # or: tools\package.ps1
```

See [INSTALL.md](INSTALL.md) for end-user install steps.

## Adding more characters
1. Add the `*.master.definition.json` to `definitions/`. Include an explicit
   `background` block (or the theme will fall back to a name-based search):
   ```json
   "background": { "name": "my_char.png", "anchor": "center" }
   ```
2. Add entries for the new sticker / wallpaper file names to the
   `RemotePathLookup` table in `src/doki/theme.cpp` so the asset manager
   knows the authoritative CDN paths.
3. (Optional) Pre-warm the local cache for offline use:
   ```powershell
   tools\fetch_assets.ps1
   ```
4. Re-run `tools\deploy.ps1`. The new character appears in the picker; the
   plugin downloads its assets on first apply.

## Cross-platform notes
The engine is plain C++ + IDA SDK (no Qt linkage in tty mode), so it ports
cleanly. Qt and `Qt6::Network` are used by the sticker overlay and the asset
manager; the rest is portable.
- **Output name:** `doki_theme.dll` (Windows) / `.so` (Linux) / `.dylib` (macOS)
  — handled by `ida_add_plugin`.
- **`$IDAUSR` default:** `%APPDATA%\Hex-Rays\IDA Pro` (Windows),
  `~/.idapro` (Linux/macOS). All paths resolve via `get_user_idadir()`.
- **Theme activation:** uses the `ThemeName` registry/config value through
  the SDK registry API, which is abstracted per-platform by IDA.
- Path separator handling is centralized in `src/doki/paths.cpp`.

Only Windows is built/tested so far; Linux/macOS should build with the same
CMake against their SDKs.

## Attribution
Theme palettes and character art come from the **doki-theme** project by
Unthrottled and contributors — see
[`assets/ATTRIBUTION.md`](assets/ATTRIBUTION.md) and the
[upstream asset repository](https://github.com/doki-theme/doki-theme-assets).
Character artwork belongs to the respective studios/artists.

## License
Plugin code: MIT (see LICENSE). Bundled doki assets retain their upstream terms.
