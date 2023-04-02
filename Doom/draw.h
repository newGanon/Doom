#pragma once
#include "util.h"

typedef struct {
	u32* pixels;
	u32 width;
	u32 height;
}Texture;  

void drawPixel(i32 x, i32 y, i32 color, u32* pixels);
void drawVerticalLine(i32 x, i32 y0, i32 y1, u32 color, u32* pixels);
void drawLine(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels);
void drawSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels);
void fillSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels);
void fillRectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels);
void drawCircle(i32 x0, i32 y0, i32 a, i32 b, u32 color, u32* pixels);
void draw3D(Player player, Map map, u32* pixels);

u32 changeRGBBrightness(u32 color, f32 factor);
f32 calcShade(v2 start, v2 end);