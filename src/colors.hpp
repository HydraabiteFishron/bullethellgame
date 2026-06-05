#pragma once

#include <SDL3/SDL.h>

namespace Colors {
	constexpr SDL_Color white           = { 255, 255, 255, 255 };
	constexpr SDL_Color black           = {   0,   0,   0, 255 };
	constexpr SDL_Color red             = { 255,   0,   0, 255 };
    constexpr SDL_Color hatsune_miku    = {  51, 187, 173, 255 };
    constexpr SDL_Color tokyonight_blue = {  22,  22,  30, 255 };
}

inline void SetDrawColor(SDL_Renderer *rr, SDL_Color col = Colors::white) {
	SDL_SetRenderDrawColor(rr, col.r, col.g, col.b, col.a);
}
