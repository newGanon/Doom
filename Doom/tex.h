#pragma once
#include "util.h"
#include <SDL.h>

void init_tex(Texture* tex1);
void load_textures(SDL_Surface** surfaces);
Texture* get_texture(i32 index);
