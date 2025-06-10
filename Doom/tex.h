#pragma once
#include "util.h"
#include <SDL.h>


typedef struct Palette_t {
	// colors has 256, each with 32 shades
	u32 colors[256][32];
	u32 color_amt;
	u32 shade_amt;
} Palette;


typedef struct LightmapindexTexture_t {
	u8* indices;
	u32 width;
	u32 height;
}LightmapindexTexture;


void init_tex(SDL_Surface** surfaces, Palette* lightmap, LightmapindexTexture* index_textures);
void load_textures(SDL_Surface** surfaces);
