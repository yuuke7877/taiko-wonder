# Taiko Wonder song charts

The game currently loads `.twn` text charts from the `songs/` folder beside the executable. Starter charts are included for all seven genres. The matching `.sin` files are editable human-readable source copies using the requested `.sin` extension; keep the corresponding `.twn` file beside them so the current game loader can play that chart.

## Text chart format

```ini
# comments start with #
title=My practice song
genre=0
notes=D K D D K
```

Genre values: 0 Pop, 1 Anime, 2 Vocaloid, 3 Game Music, 4 Variety, 5 Classical, 6 Namco Original. `D` means Don and `K` means Ka. These are original practice patterns, not licensed music or official charts. The `.sin` extension is a plain-text Taiko Wonder chart format and is not compatible with unrelated commercial simulator formats.
