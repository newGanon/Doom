#include <math.h>

#include "draw.h"
#include "math.h"


void drawPixel(i32 x, i32 y, i32 color, u32* pixels) {
	if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && (color & 0xFF000000) != 0) {
		pixels[y * SCREEN_WIDTH + x] = color;
	}
}

void drawVerticalLine(i32 x, i32 y0, i32 y1, u32 color, u32* pixels) {
	if (y0 > y1) {
		i32 temp = y0;
		y0 = y1;
		y1 = temp;
	}
	for (i32 y = y0; y <= y1; y++) {
		drawPixel(x, y, color, pixels);
	}
}

/* Bresenham's line algorithm */
void drawLine(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		drawPixel(x0, y0, color, pixels);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 > -dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void drawSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels) {
	//right and left line
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		drawPixel(x0, y, color, pixels);
		drawPixel(x0 + size, y, color, pixels);
	}
	//top and bottom line
	for (i32 x = x0; x < (x0 + (i32)size); x++) {
		drawPixel(x, y0, color, pixels);
		drawPixel(x, y0 + size, color, pixels);
	}
}

void fillSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels) {
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		for (i32 x = x0; x < (x0 + (i32)size); x++) {
			drawPixel(x, y, color, pixels);
		}
	}
}

void fillRectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels) {
	i32 temp;
	if (x0 > x1) {
		temp = x0;
		x0 = x1;
		x1 = temp;
	}
	if (y0 > y1) {
		temp = y0;
		y0 = y1;
		y1 = temp;
	}
	for (i32 y = y0; y < y1; y++) {
		for (i32 x = x0; x < x1; x++) {
			drawPixel(x, y, color, pixels);
		}
	}
}

/* Bresenham's circle algorithm */
void drawCircle(i32 x0, i32 y0, i32 a, i32 b, u32 color, u32* pixels) {
	i32 dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
	i64 a2 = a * a, b2 = b * b;
	i64 err = b2 - (2 * b - 1) * a2, e2; /* Fehler im 1. Schritt */
	do {
		drawPixel(x0 + dx, y0 + dy, color, pixels);
		drawPixel(x0 + dx, y0 - dy, color, pixels);
		drawPixel(x0 - dx, y0 - dy, color, pixels);
		drawPixel(x0 - dx, y0 + dy, color, pixels);
		e2 = 2 * err;
		if (e2 < (2 * dx + 1) * b2) { ++dx; err += (2 * dx + 1) * b2; }
		if (e2 > -(2 * dy - 1) * a2) { --dy; err -= (2 * dy - 1) * a2; }
	} while (dy >= 0);
	/* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
	while (dx++ < a) {
		/* Spitze der Ellipse vollenden */
		drawPixel(x0 + dx, y0, color, pixels);
		drawPixel(x0 - dx, y0, color, pixels);
	}
}

u32 changeRGBBrightness(u32 color, f32 factor) {
	i32 a = (color & 0xFF000000);
	i32 r = (color & 0x00FF0000) >> 16;
	i32 g = (color & 0x0000FF00) >> 8;
	i32 b = color & 0x000000FF;
	return a | (i32)(r / factor) << 16 | (i32)(g / factor) << 8 | (i32)(b / factor);
}