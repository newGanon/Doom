#pragma once
#include "util.h"
#include "sdl.h"
#include "entity.h"

void drawInit(u32* pixels);
void draw3D(Player player, Texture* tex, EntityHandler* h);
