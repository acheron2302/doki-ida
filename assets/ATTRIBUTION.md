# Asset Attribution

The character stickers, full-listing wallpapers, and theme color
definitions bundled with this plugin originate from the **doki-theme**
project by Alex Simons (Unthrottled) and contributors, and are reused
here under that project's terms.

- Theme definitions: https://github.com/doki-theme/doki-master-theme
  (`definitions/**/**.master.definition.json`)
- Sticker artwork: https://github.com/doki-theme/doki-theme-assets
  (`stickers/vscode/**`)
- Wallpaper artwork: https://github.com/doki-theme/doki-theme-assets
  (`backgrounds/**`)

The plugin does not bundle image files. The asset manager downloads
the official PNGs on first use from
`https://doki.assets.unthrottled.io` and caches them locally, so the
on-disk binaries come directly from the upstream doki-theme-assets
repository and remain under that project's terms.

Character artwork is the property of the respective anime studios /
original artists; doki-theme distributes it for theming purposes.

Bundled sticker filenames (resolved at runtime against the upstream
asset repository by `src/doki/theme.cpp`):

| File                    | Source path (doki-theme-assets)                         |
|-------------------------|---------------------------------------------------------|
| rem.png                 | stickers/vscode/reZero/rem/rem.png                      |
| zero_two_obsidian.png   | stickers/vscode/franxx/zeroTwo/obsidian/zero_two_obsidian.png |
| zero_two_sakura.png     | stickers/vscode/franxx/zeroTwo/sakura/zero_two_sakura.png |
| darkness_dark.png       | stickers/vscode/konoSuba/darkness/dark/darkness_dark.png |
| darkness_light.png      | stickers/vscode/konoSuba/darkness/light/darkness_light.png |

Bundled wallpaper filenames (transparent variant — the artwork
sits on a transparent canvas so the doki palette + code listing show
through wherever it is empty):

| File                    | Source path (doki-theme-assets)                         |
|-------------------------|---------------------------------------------------------|
| rem.png                 | backgrounds/wallpapers/transparent/rem.png              |
| zero_two_obsidian.png   | backgrounds/wallpapers/transparent/zero_two_obsidian.png |
| zero_two_sakura.png     | backgrounds/wallpapers/transparent/zero_two_sakura.png  |
| darkness_dark.png       | backgrounds/wallpapers/transparent/darkness_dark.png    |
| darkness_light.png      | backgrounds/wallpapers/transparent/darkness_light.png   |
