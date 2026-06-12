# Installing Doki Theme for IDA

## Requirements
- IDA Pro 9.0+ (developed and tested against IDA 9.4 / SDK 9.3, x64).

## Install
1. Copy `plugins\doki_theme.dll` into your IDA `plugins` directory, either:
   - the IDA install dir: `<IDADIR>\plugins\`, or
   - your user dir: `%APPDATA%\Hex-Rays\IDA Pro\plugins\` (`$IDAUSR`).
2. Copy the `doki-theme\` folder (definitions + assets) into your user dir:
   `%APPDATA%\Hex-Rays\IDA Pro\doki-theme\`
   so you end up with:
   ```
   %APPDATA%\Hex-Rays\IDA Pro\
     plugins\doki_theme.dll           (if you chose the user dir)
     doki-theme\definitions\*.json
     doki-theme\assets\stickers\*.png
   ```
3. Start IDA and open a database.

## Use
Open the **Options** menu:
- **Doki Theme: Pick character...** — choose a character from the list.
- **Doki Theme: Random character** / **Next character** — cycle quickly.
- **Doki Theme: Toggle sticker** — show/hide the character artwork.
- **Doki Theme: Restore default** — go back to your previous IDA theme.

Selecting a character:
- updates the **navigation band** colors immediately, and
- writes an IDA theme under `%APPDATA%\Hex-Rays\IDA Pro\themes\doki-<name>\`
  and activates it. The full **UI chrome + disassembly listing** colors and the
  **sticker** apply after you **restart IDA** (or reselect the theme in
  *Options ▸ Colors*), because IDA loads theme CSS at startup.

Your selected character and sticker preference persist across restarts
(`%APPDATA%\Hex-Rays\IDA Pro\doki-theme\config.json`).

## Uninstall
Remove `plugins\doki_theme.dll`, the `doki-theme\` folder, and any
`themes\doki-*` folders. Use *Options ▸ Colors* (or **Restore default**) to pick
a non-doki theme.
