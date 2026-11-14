# Visual Studio project

`taiko-wonder.vcxproj` builds the SDL2 game with Visual Studio 2022 (x64). Install Visual Studio's **Desktop development with C++** workload and vcpkg, set `VCPKG_ROOT`, then open the project or solution folder. The included `vcpkg.json` manifest specifies SDL2 and SDL2_ttf.

The repository currently has one `.sin` example chart: `songs/wonder_practice.sin`. It is an original Taiko Wonder text format. It is not yet parsed by the game runtime; current runtime charts are generated practice patterns / `.twn` charts.
