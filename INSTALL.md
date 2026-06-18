# Installing Doki Theme for IDA

## Requirements
- IDA Pro 9.0+ (developed and tested against IDA 9.4 / SDK 9.3, x64).
- Internet access on the first apply of each theme (subsequent applies
  work offline from the local cache).

## Install
1. Copy `plugins\doki_theme.dll` into your IDA `plugins` directory, either:
   - the IDA install dir: `<IDADIR>\plugins\`, or
   - your user dir: `%APPDATA%\Hex-Rays\IDA Pro\plugins\` (`$IDAUSR`).
2. Copy the `doki-theme\definitions` folder into your user dir:
   ```
   %APPDATA%\Hex-Rays\IDA Pro\
     plugins\doki_theme.dll           (if you chose the user dir)
     doki-theme\definitions\*.json
   ```
   Sticker and wallpaper images are NOT shipped in the package; the plugin
   downloads them on first use.
3. Start IDA and open a database.

## First-use asset download
The first time you apply a character, the plugin downloads its sticker
and full-listing wallpaper from
`https://doki.assets.unthrottled.io` and caches them under:
```
%APPDATA%\Hex-Rays\IDA Pro\doki-theme\cache\stickers\
%APPDATA%\Hex-Rays\IDA Pro\doki-theme\cache\wallpapers\
```
- Downloads are silent and bounded (15s timeout). If a fetch fails for
  any reason (offline, HTTP error, invalid file), the colors and nav
  band still apply; only the missing artwork is skipped.
- Cached assets are used offline on subsequent applies; no further network
  access is required.
- Failures log a concise warning to IDA's output window and are retried
  the next time the same theme is applied.

## Use
Open the **Options** menu:
- **Doki Theme: Pick character...** — choose a character from the list.
- **Doki Theme: Random character** / **Next character** — cycle quickly.
- **Doki Theme: Toggle sticker** — show/hide the character artwork.
- **Doki Theme: Toggle wallpaper** — show/hide the full-listing wallpaper.
- **Doki Theme: Restore default** — go back to your previous IDA theme.

Selecting a character:
- downloads the artwork on first use (cached for the next time),
- updates the **navigation band** colors immediately, and
- writes an IDA theme under `%APPDATA%\Hex-Rays\IDA Pro\themes\doki-<name>\`
  and activates it. The full **UI chrome + disassembly listing** colors
  apply after you **restart IDA** (or reselect the theme in
  *Options ▸ Colors*), because IDA loads theme CSS at startup.

Your selected character, sticker and wallpaper preferences persist
across restarts (`%APPDATA%\Hex-Rays\IDA Pro\doki-theme\config.json`).

## Uninstall
Remove `plugins\doki_theme.dll`, the `doki-theme\` folder, and any
`themes\doki-*` folders. Use *Options ▸ Colors* (or **Restore default**)
to pick a non-doki theme.
