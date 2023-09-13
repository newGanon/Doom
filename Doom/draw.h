#pragma once
#include "util.h"
#include "sdl.h"
#include "entity.h"

typedef struct Texture {
	u32* pixels;
	u32 width;
	u32 height;
}Texture;  

void drawInit(u32* pixels);
void draw3D(Player player, Texture* tex, EntityHandler* h);
