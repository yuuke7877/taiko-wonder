# Taiko Wonder songs

Place original user-created `.twn` chart files in the `songs/` folder beside the executable. Files are UTF-8 text. Example:

```ini
# Original practice chart
 title=My practice song
genre=0
notes=D K D D K
```

Genres are numbered: 0 Pop, 1 Anime, 2 Vocaloid, 3 Game Music, 4 Variety, 5 Classical, 6 Namco Original. Supported note tokens are `D` (Don) and `K` (Ka), separated by whitespace. Files with no usable notes are ignored. If no valid charts are found, the game provides generated practice charts. No copyrighted songs or official charts are included.
