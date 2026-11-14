# Taiko Wonder song charts

The project includes one editable `.sin` example at `songs/wonder_practice.sin`. This is Taiko Wonder's own simple text format, not a chart format compatible with another simulator. It is an original practice pattern and contains no copyrighted song data.

Example properties:

```ini
title=Wonder Practice
artist=Taiko Wonder Community
genre=0
bpm=120
offset_ms=0
notes=D K D D K
```

Genre IDs: 0 Pop, 1 Anime, 2 Vocaloid, 3 Game Music, 4 Variety, 5 Classical, 6 Namco Original. `D` means Don and `K` means Ka. The current game runtime loads `.twn` charts; `.sin` is supplied as the requested editable source example and requires a converter/runtime parser before it can be played directly.
