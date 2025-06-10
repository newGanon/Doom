#pragma once
#include "util.h"
#include "sdl.h"
#include "entity.h"
#include "tex.h"

void draw_init(u32* pixels, Palette* lightmap1, LightmapindexTexture* index_textures1);
void draw_3d(Player player, EntityHandler* h);
