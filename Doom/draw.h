#pragma once
#include "util.h"
#include "sdl.h"
#include "entity.h"

void draw_init(u32* pixels);
void draw_3d(Player player, Texture* tex, EntityHandler* h);
