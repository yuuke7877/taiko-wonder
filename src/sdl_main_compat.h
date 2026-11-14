#pragma once

// Use the application's regular main() instead of SDL2main's WinMain wrapper.
#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace taiko_wonder_sdl_startup {
struct MainReady {
    MainReady() { SDL_SetMainReady(); }
};
// SDL_SetMainReady must run before SDL_Init; this static initializer does so
// before the program enters main().
static MainReady mainReady;
}
