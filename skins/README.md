# Taiko Wonder skins

The game reads the `skins/` folder beside the executable. It uses `skins/default` when `TAIKO_WONDER_SKIN` is not set or the chosen skin is invalid. Add a new subfolder and `skin.ini`; no SimpleStyle assets are copied or required.

Supported keys:

```ini
background_top=#ffdd44
background_bottom=#ff844c
accent=#ff5e77
panel=#ffffff
text=#332336
font=OptionalFont.ttf
```

A skin may also include `background.bmp` (uncompressed BMP). `font` is a path relative to the skin folder. To select a skin, set `TAIKO_WONDER_SKIN` to a folder name under `skins`, or to a full path. The default palette and generated SDL background remain active if a skin is missing or invalid.
