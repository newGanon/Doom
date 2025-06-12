#include "tex.h"
#include <SDL.h>

static u32 tex_amt;

void tex_load(SDL_Surface** surfaces) {
	SDL_Surface* bmpTex;
	char textureFileNames[4][50] = { "Assets/test.bmp", "Assets/spritetest2.bmp", "Assets/ammo.bmp", "Assets/test3.bmp"};
	tex_amt = sizeof(textureFileNames) / sizeof(textureFileNames[0]);

	for (u32 i = 0; i < tex_amt; i++) {
		bmpTex = SDL_LoadBMP(textureFileNames[i]);
		ASSERT(bmpTex, "Error loading texture %s\n", SDL_GetError());
		surfaces[i] = bmpTex;
	}
}


void create_lightmap(Palette* pal) {
	// "web-safe" palette, but 0 is a special value for transparent color, and there are only 39 gray scale colors at the end
	// 0 : special value for trasparent 
	// 1-217 : colors
	// 218-255 : gray-scale colors
	u8 levels[] = {0, 51, 102, 153, 204, 255};
	u32(*colors)[32] = (u32(*)[32])pal->colors;
	// first color is special color for transparent pixels
	for (usize i = 0; i < 32; i++) colors[0][i] = 0;
	// normal colors
	for (usize x = 0; x < 6; x++) {
		for (usize y = 0; y < 6; y++) {
			for (usize z = 0; z < 6; z++) {
				u8 r = levels[x];
				u8 g = levels[y];
				u8 b = levels[z];
				f32 r_step = r / 31.0f;
				f32 g_step = g / 31.0f;
				f32 b_step = b / 31.0f;
				usize color_number = z + (6 * y) + (6 * 6 * x) + 1;
				// 32 shades
				for (usize s = 0; s < 32; s++) {
					u32 c = (0xFF << 24) | ((r - (i32)(r_step * s)) << 16) | ((g - (i32)(g_step * s)) << 8) | ((b - (i32)(b_step * s)));
					colors[color_number][s] = c;
				}
			}
		}
	}
	// 39 tones of gray
	for (usize i = 1; i < 40; i++) {
		u8 gray = (u8)((255 / 39.0f) * i);
		f32 gray_step = gray / 32.0f;
		for (usize s = 0; s < 32; s++) {
			colors[6 * 6 * 6 + i][s] = (11111111 << 24) | ((gray - (i32)(gray_step * s)) << 16) | ((gray - (i32)(gray_step * s)) << 8) | ((gray - (i32)(gray_step * s)));
		}
	}
}


// only first element of each array is used, as this is the normal unshaded color
u8 find_closest_color_index(u32 color, const u32 palette[256][32]) {
	if (A(color) == 0) return 0;
	i32 closest_index = 0;
	i32 min_distance = INT_MAX;

	i32 r1 = R(color);
	i32 g1 = G(color);
	i32 b1 = B(color);

	for (i32 i = 1; i < 256; ++i) {
		i32 r2 = R(palette[i][0]);
		i32 g2 = G(palette[i][0]);
		i32 b2 = B(palette[i][0]);

		i32 dr = r1 - r2;
		i32 dg = g1 - g2;
		i32 db = b1 - b2;

		i32 distance = dr * dr + dg * dg + db * db;  // No sqrt needed

		if (distance < min_distance) {
			min_distance = distance;
			closest_index = i;
		}
	}

	return closest_index;
}


void create_index_textures(SDL_Surface** surfaces, Palette* pal, LightmapindexTexture* index_textures) {
	for (u32 index = 0; index < tex_amt; ++index) {
		SDL_Surface* cur_sur = surfaces[index];
		LightmapindexTexture* cur_index_array = &index_textures[index];

		i32 width = cur_sur->w;
		i32 height = cur_sur->h;

		cur_index_array->width = width;
		cur_index_array->height = height;

		if (width <= 0 || height <= 0 || (size_t)width * (size_t)height > SIZE_MAX) {
			fprintf(stderr, "width and height of texture not valid at surface %d\n", index);
			continue;
		}

		u32* surface_pixels = (u32*)cur_sur->pixels;
		cur_index_array->indices = (u8*)malloc(sizeof(u8) * width * height);
		if (!cur_index_array->indices) {
			fprintf(stderr, "Memory allocation failed for index_array at surface %d\n", index);
			continue;
		}

		for (i32 y = 0; y < height; ++y) {
			for (i32 x = 0; x < width; ++x) {
				u32 color = surface_pixels[y * width + x];
				u8 nearest_index = find_closest_color_index(color, pal->colors);
				cur_index_array->indices[y * width + x] = nearest_index;
			}
		}
	}
}


void tex_init(SDL_Surface** surfaces, Palette* lightmap, LightmapindexTexture* index_textures) {
	tex_load(surfaces);
	create_lightmap(lightmap);
	create_index_textures(surfaces, lightmap, index_textures);
}


void tex_free(SDL_Surface** surfaces, LightmapindexTexture* index_textures) {
	for (u32 i = 0; i < tex_amt; i++) {
		SDL_FreeSurface(surfaces[i]);
		if (index_textures[i].width <= 0 || index_textures[i].height <= 0 || (size_t)index_textures[i].width * (size_t)index_textures[i].height > SIZE_MAX) { continue; }
		free(index_textures[i].indices);
	}
}