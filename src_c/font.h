#ifndef PGFONT_INTERNAL_H
#define PGFONT_INTERNAL_H

#ifdef PG_SDL3
#include <SDL3_ttf/SDL_ttf.h>

// SDL3_ttf uses SDL3 error reporting API
#define TTF_GetError SDL_GetError

#else
#include <SDL_ttf.h>
#endif

#include "include/pygame_font.h"

#endif /* ~PGFONT_INTERNAL_H */
