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
		drawPixel(x, SCREEN_HEIGHT - y, color, pixels);
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

void draw3D(Player player, Map map, u32* pixels) {

	u16 ceilingclip[SCREEN_WIDTH];
	u16 floorclip[SCREEN_WIDTH];

	u8 renderedSectors[SECTOR_MAX];
	memset(renderedSectors, 0, sizeof(renderedSectors));

	for (int i = 0; i < SCREEN_WIDTH; i++) {
		ceilingclip[i] = SCREEN_HEIGHT - 1;
		floorclip[i] = 0;
	}

	v2  zdl = v2Rotate(((v2) { 0.0f, 1.0f }), +(HFOV / 2.0f)),
		zdr = v2Rotate(((v2) { 0.0f, 1.0f }), -(HFOV / 2.0f)),
		znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR },
		znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR },
		zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR },
		zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR };

	enum { MaxQueue = 64 };
	struct item { int sectorno, sx1, sx2; } queue[MaxQueue], *head = queue, *tail = queue;

	*head = (struct item){ player.sector, 0, SCREEN_WIDTH - 1 };
	if (++head == queue + MaxQueue) head = queue;

	v2 p1, p2;
	while (head != tail) {
		const struct item now = *tail;
		if (++tail == queue + MaxQueue) tail = queue;

		if (renderedSectors[now.sectorno - 1] == 1) continue;

		++renderedSectors[now.sectorno - 1];

		Sector sec = map.sectors[now.sectorno - 1];
		for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {

			//world pos
			p1 = world_pos_to_camera(v2Mul(map.walls[i].a, 1.0f), player);
			p2 = world_pos_to_camera(v2Mul(map.walls[i].b, 1.0f), player);

			f32 a1 = normalize_angle(atan2(p1.y, p1.x) - PI / 2);
			f32 a2 = normalize_angle(atan2(p2.y, p2.x) - PI / 2);

			//calculate intersection between walls and view frustum and clip walls
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
			if (a1 < a2 || a2 < -(HFOV / 2) - 0.0001f || a1 > +(HFOV / 2) + 0.0001f) continue;

			//convert the angle of the wall into screen coordinates (player FOV is 90 degrees or 1/2 PI)
			f32 x1 = clamp(screen_angle_to_x(a1), now.sx1, now.sx2);
			f32 x2 = clamp(screen_angle_to_x(a2), now.sx1, now.sx2);

			//get floor and ceiling height of sector behind wall if wall is a portal
			i32 nzfloor = 0;
			i32 nzceil = 0;

			if (map.walls[i].portal != 0) {
				nzfloor = map.sectors[map.walls[i].portal - 1].zfloor;
				nzceil = map.sectors[map.walls[i].portal - 1].zceil;
			}

			f32 sy0 = (VFOV * SCREEN_HEIGHT) / p1.y;
			f32 sy1 = (VFOV * SCREEN_HEIGHT) / p2.y;

			//wall coordinates
			i32 yf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player.pos.z) * sy0);
			i32 yf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player.pos.z) * sy1);
			i32 yc0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player.pos.z) * sy0);
			i32 yc1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player.pos.z) * sy1);
			//portal coordinates in the wall
			i32 pf0 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player.pos.z) * sy0);
			i32 pf1 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player.pos.z) * sy1);
			i32 pc0 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player.pos.z) * sy0);
			i32 pc1 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player.pos.z) * sy1);


			u32 color = (map.walls[i].portal) ? BLUE : RED;

			for (i32 x = x1; x < x2; x++) {
				//calculate x stepsize
				f32 xp = (x - x1) / (f32)(x2 - x1);

				if (map.walls[i].portal != 0) {
					i32 d = 32;
				}

				//get top and bottom coordinates of the wall
				i32 yf = clamp((i32)(xp * (yf1 - yf0)) + yf0, floorclip[x], ceilingclip[x]);
				i32 yc = clamp((i32)(xp * (yc1 - yc0)) + yc0, floorclip[x], ceilingclip[x]);

				//draw floor
				if (yf > floorclip[x]) { drawVerticalLine(x, floorclip[x], yf, ORANGE, pixels); }
				//draw ceiling
				if (yc < ceilingclip[x]) { drawVerticalLine(x, yc, ceilingclip[x], PURPLE, pixels); }

				//draw Wall
				if (map.walls[i].portal == 0) { drawVerticalLine(x, yf, yc, color, pixels); }
				//draw Portal
				else {
					//get top and bottom coordinates of the portal
					i32 pyf = clamp((i32)(xp * (pf1 - pf0)) + pf0, yf, yc);
					i32 pyc = clamp((i32)(xp * (pc1 - pc0)) + pc0, yf, yc);

					//if neighborfloor is higher than current sectorceiling then draw it
					if (pyf > yf) { drawVerticalLine(x, yf, pyf, YELLOW, pixels); }
					//draw window
					//drawVerticalLine(x, pyf, pyc, color, pixels);
					//if neighborceiling is lower than current sectorceiling then draw it
					if (pyc < yc) { drawVerticalLine(x, pyc, yc, GREEN, pixels); }

					//update vertical clipping arrays
					ceilingclip[x] = clamp(pyc, 0, SCREEN_HEIGHT - 1);
					floorclip[x] = clamp(pyf, 0, SCREEN_HEIGHT - 1);
				}
			}
			if (map.walls[i].portal) {
				*head = (struct item){ map.walls[i].portal, x1, x2 };
				if (++head == queue + MaxQueue) head = queue;
			}
		}
	}
}
