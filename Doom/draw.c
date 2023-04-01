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
	i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	i32 dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	i32 err = (dx > dy ? dx : -dy) / 2, e2;

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

void draw3D(Player player, u32* pixels) {
	v2 test1 = { 40 , 200 };
	v2 test2 = { 40 , 10 };
	f32 zceil = 10;
	f32 zfloor = 0;

	const v2
		zdl = v2Rotate(((v2) { 0.0f, 1.0f }), +(HFOV / 2.0f)),
		zdr = v2Rotate(((v2) { 0.0f, 1.0f }), -(HFOV / 2.0f)),
		znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR },
		znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR },
		zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR },
		zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR };

	//world pos
	v2 p1 = world_pos_to_camera(test1, player); 
	v2 p2 = world_pos_to_camera(test2, player);

	f32 a1 = normalize_angle(atan2(p1.y, p1.x) - PI/2);
	f32 a2 = normalize_angle(atan2(p2.y, p2.x) - PI/2);


	if (p1.y < ZNEAR || p2.y < ZNEAR || a1 > +(HFOV / 2) || a2 < -(HFOV / 2)) {
		v2 il;
		i32 hitl = get_line_intersection(p1, p2, znl, zfl, &il);
		v2 ir;
		i32 hitr = get_line_intersection(p1, p2, znr, zfr, &ir);
		if (hitl) {
			p1 = il;
			a1 = normalize_angle(atan2(p1.y, p1.x) - PI / 2);
		}
		if (hitr) {
			p2 = ir;
			a2 = normalize_angle(atan2(p2.y, p2.x) - PI / 2);
		}
	}
	if (a1 < a2) return;
	if (a2 < -(HFOV / 2) - 0.0001f || a1 > +(HFOV / 2) + 0.0001f) return;

	f32 x1 = screen_angle_to_x(a1);
	f32 x2 = screen_angle_to_x(a2);

	f32 sy0 = (VFOV * SCREEN_HEIGHT) / p1.y;
	f32 sy1 = (VFOV * SCREEN_HEIGHT) / p2.y;

	i32 yf0 = (SCREEN_HEIGHT / 2) + (i32)((zfloor + EYEHEIGHT) * sy0);
	i32 yf1 = (SCREEN_HEIGHT / 2) + (i32)((zfloor + EYEHEIGHT) * sy1);
	i32 yc0 = (SCREEN_HEIGHT / 2) + (i32)((zceil + EYEHEIGHT) * sy0);
	i32 yc1 = (SCREEN_HEIGHT / 2) + (i32)((zceil + EYEHEIGHT) * sy1);

	drawLine(SCREEN_WIDTH/2 - p1.x + 400, SCREEN_HEIGHT/2 - p1.y +200, SCREEN_WIDTH / 2 - p2.x + 400, SCREEN_HEIGHT / 2 - p2.y+ 200, RED, pixels);
	drawPixel(SCREEN_WIDTH / 2 + 400, SCREEN_HEIGHT / 2+ 200, RED, pixels);

	for (i32 x = x1; x > x2; x--) {
		f32 xp = (x - x1) / (f32)(x2 - x1);

		i32 yf = (i32)(xp * (yf1 - yf0)) + yf0;
		yf = clamp(yf, 0, SCREEN_HEIGHT - 1);
		i32 yc = (i32)(xp * (yc1 - yc0)) + yc0;
		yc = SCREEN_HEIGHT - clamp(yc, 0, SCREEN_HEIGHT - 1);
		drawVerticalLine(x, yf, yc, RED, pixels);
	}
}
