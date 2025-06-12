#pragma once
#include "util.h"

typedef struct EntityHandler_s EntityHandler;
typedef struct Player_s Player;
typedef struct LightmapindexTexture_t LightmapindexTexture;
typedef struct Palette_t Palette;

void draw_init(u32* pixels, Palette* lightmap1, LightmapindexTexture* index_textures1);
void draw_3d(Player* player, EntityHandler* h);
void draw_2d();