#include "tex.h";

Texture* tex;

void init_tex(Texture* tex1) {
	tex = tex1;
}

void load_textures(SDL_Surface** surfaces) {
	SDL_Surface* bmpTex;
	char textureFileNames[3][50] = { "test3.bmp", "spritetest2.bmp", "ammo.bmp" };
	i32 textureAmt = sizeof(textureFileNames) / sizeof(textureFileNames[0]);

	for (i32 i = 0; i < textureAmt; i++) {
		bmpTex = SDL_LoadBMP(textureFileNames[i]);
		ASSERT(bmpTex,
			"Error loading texture %s\n",
			SDL_GetError());
		tex[i] = (Texture){ (u32*)bmpTex->pixels, bmpTex->w, bmpTex->h };
		surfaces[i] = bmpTex;
	}
}

Texture* get_texture(i32 index) {
	return &tex[index];
}
