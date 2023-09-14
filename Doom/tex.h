#pragma once
#include "util.h"
#include <SDL.h>

void init_tex(Texture* tex1);
void loadTextures(SDL_Surface** surfaces);
Texture* get_texture(i32 index);
