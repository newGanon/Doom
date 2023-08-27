#include <math.h>
#include "draw.h"
#include "math.h"


visplane_t visplanes[MAXVISPLANES];
visplane_t* lastvisplane;
visplane_t* floorplane;
visplane_t* ceilplane;

v2  zdl, zdr, znl, znr, zfl, zfr;


void drawPixel(i32 x, i32 y, u32 color, u32* pixels) {
	if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && (color & 0xFF000000) != 0) {
		pixels[((SCREEN_HEIGHT - 1) - y) * SCREEN_WIDTH + x] = color;
	}
}

void drawInit() {
	zdl = v2Rotate(((v2) { 0.0f, 1.0f }), +(HFOV / 2.0f)),
	zdr = v2Rotate(((v2) { 0.0f, 1.0f }), -(HFOV / 2.0f)),
	znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR },
	znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR },
	zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR },
	zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR };
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

void draw3D(Player player, Map* map, u32* pixels, Texture* tex) {

	clearPlanes();

	drawWall3D(player, map, pixels, tex, &(WallRenderingInfo) { player.sector, 0, SCREEN_WIDTH, { 0 }}, 0);

	//drawPlanes3D(player, map, pixels, tex);

	drawSprites(player, map, pixels, tex);

	drawMinimap(player, map, pixels);
}

void drawWall3D(Player player, Map* map, u32* pixels, Texture* tex, WallRenderingInfo* now, u32 rd)
{
	if (rd > 32) return;
	Sector sec = map->sectors[now->sectorno - 1];
	for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {


		//world pos
		v2 p1 = world_pos_to_camera(map->walls[i].a, player);
		v2 p2 = world_pos_to_camera(map->walls[i].b, player);

		v2 tp1 = p1;
		v2 tp2 = p2;

		f32 a1 = atan2(p1.y, p1.x) - PI / 2;
		f32 a2 = atan2(p2.y, p2.x) - PI / 2;

		//calculate intersection between walls and view frustum and clip walls
		if (p1.y < ZNEAR || p2.y < ZNEAR || a1 > +(HFOV / 2) || a2 < -(HFOV / 2)) {
			v2 il;
			i32 hitl = get_line_intersection(p1, p2, znl, zfl, &il);
			v2 ir;
			i32 hitr = get_line_intersection(p1, p2, znr, zfr, &ir);
			if (hitl) {
				p1 = il;
				a1 = atan2(p1.y, p1.x) - PI / 2;
			}
			if (hitr) {
				p2 = ir;
				a2 = atan2(p2.y, p2.x) - PI / 2;
			}
		}
		if (a1 < a2 || a2 < -(HFOV / 2) - 0.0001f || a1 > +(HFOV / 2) + 0.0001f) continue;

		//convert the angle of the wall into screen coordinates (player FOV is 90 degrees or 1/2 PI)
		i32 tx1 = screen_angle_to_x(a1);
		i32 tx2 = screen_angle_to_x(a2);

		if (tx1 > now->sx2) continue;
		if (tx2 < now->sx1) continue;

		i32 x1 = clamp(tx1, now->sx1, now->sx2);
		i32 x2 = clamp(tx2, now->sx1, now->sx2);


		//rempove part of wall that is already covered by wall, works because we sort walls from near to far
		for (i32 i = x1; i < x2; i++) {
			if (ceilingclip[i] == 0) {
				x1 = i+1;
			}
			else break;
		}

		for (i32 i = x2; i > x1; i--) {
			if (ceilingclip[i] == 0) {
				x2 = i;
			}
			else break;
		}

		floorplane = findPlane(sec.zfloor, 0);
		ceilplane = findPlane(sec.zceil, 0);

		floorplane = checkPlane(floorplane, x1, x2);
		ceilplane = checkPlane(ceilplane, x1, x2);

		//get floor and ceiling height of sector behind wall if wall is a portal
		i32 nzfloor = sec.zfloor;
		i32 nzceil = sec.zceil;

		if (map->walls[i].portal != 0) {
			nzfloor = map->sectors[map->walls[i].portal - 1].zfloor;
			nzceil = map->sectors[map->walls[i].portal - 1].zceil;
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

		//u32 color = (map->walls[i].portal) ? BLUE : RED;

		//wall texture mapping varaibles
		v2 difp1 = v2Sub(p1, tp1);
		v2 difp2 = v2Sub(p2, tp2);
		f32 twlen = v2Len(v2Sub(tp1, tp2));
		v2 cutoff = { fabsf(v2Len(difp1) / twlen), fabsf(v2Len(difp2) / twlen) };

		//TODO: remove varialbe
		Wall wall = map->walls[i];
		for (i32 x = x1; x < x2; x++) {
			//calculate x stepsize
			f32 xp = (x - tx1) / (f32)(tx2 - tx1);

			//get top and bottom coordinates of the wall
			i32 tyf = (i32)(xp * (yf1 - yf0)) + yf0;
			i32 tyc = (i32)(xp * (yc1 - yc0)) + yc0;
			i32 yf = clamp(tyf, floorclip[x], ceilingclip[x]);
			i32 yc = clamp(tyc, floorclip[x], ceilingclip[x]);


			//draw floor
			// if (yf > floorclip[x]) { drawVerticalLine(x, map->floorclip[x], yf, changeRGBBrightness(ORANGE, 1.0f + (f32)((i32)sec.zceil % 10) / 10.0f), pixels); } 

			//draw ceiling
			//if (yc < ceilingclip[x]) { drawVerticalLine(x, yc, map->ceilingclip[x], changeRGBBrightness(PURPLE, 1.0f + (f32)((i32)sec.zceil % 10) / 10.0f), pixels); } 

			//used lookuptables screenxtoangle and yslope to make rendering flats faster
			Texture floorTex = tex[0];
			Texture ceilTex = tex[0];
			//floor
			for (i32 y = floorclip[x]; y < yf; y++) {
				f32 a = screenxtoangle[x];
				//scale normalized yslope by actual camera pos and divide by the cosine of angle to prevent fisheye effect
				f32 dis = fabs(((player.pos.z - sec.zfloor) * yslope[y]) / (cos(a)));
				//relative coordinates to player
				f32 xt = -sin(a) * dis;
				f32 yt = cos(a) * dis;
				// absolute ones
				v2 p = camera_pos_to_world((v2) { xt, yt }, player);
				// texutre coordinates
				f32 pixelshade = calcFlatShade(dis);
				v2i t = { abs((i32)(fmod((p.x), 8.0f) * 32.0f)) ,abs((i32)(fmod((p.y), 8.0f) * 32.0f))};
				u32 color = changeRGBBrightness(floorTex.pixels[(256 - t.y) * floorTex.width + t.x], pixelshade);
				drawPixel(x, y, color, pixels);
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}

			//ceiling
			for (i32 y = yc; y < ceilingclip[x]; y++) {
				f32 a = screenxtoangle[x];
				//scale normalized yslope by actual camera pos and divide by the cosine of angle to prevent fisheye effect
				f32 dis = fabs(((sec.zceil - player.pos.z) * yslope[y]) / (cos(a)));
				//relative coordinates to player
				f32 xt = -sin(a) * dis;
				f32 yt = cos(a) * dis;
				// absolute ones
				v2 p = camera_pos_to_world((v2) { xt, yt }, player);
				// texutre coordinates
				f32 pixelshade = calcFlatShade(dis);
				v2i t = { (abs((i32)((p.x - (i32)p.x) * 256.0f))) ,abs((i32)((p.y - (i32)p.y) * 256.0f))};
				u32 color = changeRGBBrightness(ceilTex.pixels[(256 - t.y) * ceilTex.width + t.x], pixelshade);
				drawPixel(x, y, color, pixels);
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}

			

			//set visplane top and bottom clipping
			if (yf > floorclip[x]) {
				floorplane->bottom[x] = floorclip[x];
				floorplane->top[x] = yf;
			}

			if (yc < ceilingclip[x]) {
				ceilplane->bottom[x] = yc;
				ceilplane->top[x] = ceilingclip[x];
			}

			//variables used in wikipedia equation for texture mapping https://en.wikipedia.org/wiki/Texture_mapping
			//affine texture mapping: (1.0f-a) * u0 + a*u1
			//perspective correct texture mapping: ((1.0f-a) * u0/z0 + a*(u1/z1)) / ((1.0f-a) * 1/z0 + a*(1.0f/z1))

			//a: x part where we currently are on the wall [0...1]
			f32 a = xp;
			//u0: how much of the left part of the wall is cut off
			f32 u0 = cutoff.x;
			//u1: how much of the right part of the wall is cut off 
			f32 u1 = 1.0f - cutoff.y;
			//z0: how far away the left wallpoint is from the player
			f32 z0 = p1.y;
			//z1: how far away the right wallpoint ist from the player
			f32 z1 = p2.y;

			f64 u = ((1.0f - a) * (u0 / z0) + a * (u1 / z1)) / ((1.0f - a) * 1 / z0 + a * (1.0f / z1));


			//wall distance for lightlevel calc
			f32 dis = tp1.y * (1 - u) + tp2.y * (u);
			f32 wallshade = calcWallShade(map->walls[i].a, map->walls[i].b, dis);

			//draw Wall
			if (map->walls[i].portal == 0) {
				//drawVerticalLine(x, yf, yc, changeRGBBrightness(color, wallshade), pixels); wall in one color
				if (yc > tyf && yf < tyc) {
					drawTexLine(x, yf, yc, tyf, tyc, u, tex, wallshade, dis, pixels);
				}
				ceilingclip[x] = 0;
				floorclip[x] = SCREEN_HEIGHT - 1;
			}

			//draw Portal
			else {
				//get top and bottom coordinates of the portal
				i32 tpyf = (i32)(xp * (pf1 - pf0)) + pf0;
				i32 pyf = clamp(tpyf, yf, yc);
				i32 tpyc = (i32)(xp * (pc1 - pc0)) + pc0;
				i32 pyc = clamp(tpyc, yf, yc);

				//if neighborfloor is higher than current sectorceiling then draw it
				//if (pyf > yf) { drawVerticalLine(x, yf, pyf, changeRGBBrightness(YELLOW, wallshade), pixels); }
				if (pyf > yf) { drawTexLine(x, yf, pyf, tyf, tpyf, u, tex, wallshade, dis, pixels); for (i32 y = yf; y < pyf; y++); }
				//draw window
				//drawVerticalLine(x, pyf, pyc, color, pixels);
				//if neighborceiling is lower than current sectorceiling then draw it
				if (pyc < yc) { drawTexLine(x, pyc, yc, tpyc, tyc, u, tex, wallshade, dis, pixels); for (i32 y = pyc; y < yc; y++); }

				//update vertical clipping arrays
				ceilingclip[x] = clamp(pyc, 0, ceilingclip[x]);
				floorclip[x] = clamp(pyf, floorclip[x], SCREEN_HEIGHT - 1);
			}
		}

		if (map->walls[i].portal && !now->renderedSectors[map->walls[i].portal - 1]) {
			WallRenderingInfo* w = &(WallRenderingInfo){ map->walls[i].portal, x1, x2};
			memcpy(w->renderedSectors, now->renderedSectors, SECTOR_MAX * sizeof(u8));
			w->renderedSectors[now->sectorno - 1] = 1;
			drawWall3D(player, map, pixels, tex, w, ++rd);
		}
	}
}

f32 calcWallShade(v2 start, v2 end, f32 dis) {
	v2 difNorm = v2Normalize(v2Sub(end, start));
	return (f32)1.0f + (10.0f * (fabsf(difNorm.x)) + fabsf(dis)) * LIGHTDIMINISHINGDFACTOR;
}

f32 calcFlatShade(f32 dis) {
	return (f32)1 + fabsf(dis) * LIGHTDIMINISHINGDFACTOR;
}

void drawTexLine(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, f64 u, Texture* tex, f32 shade,f32 dis, u32* pixels) {
	i32 tx = u * tex[0].width;		
	for (i32 y = y0; y <= y1; y++) {
		f64 v = 1.0 - ((y - yf) / (f64)(yc - yf));
		i32 ty = v * tex[0].height;
		u32 color = changeRGBBrightness(tex[0].pixels[ty * tex[0].width + tx], shade);
		drawPixel(x, y, color, pixels);
		zBuffer[y * SCREEN_WIDTH + x] = dis;
	}
}

void clearPlanes() {
	//clear zBuffer
	for (i32 i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) { zBuffer[i] = ZFAR; }

	//draw Walls and floor
	for (int i = 0; i < SCREEN_WIDTH; i++) {
		ceilingclip[i] = SCREEN_HEIGHT - 1;
		floorclip[i] = 0;
	}
	lastvisplane = visplanes;
}

visplane_t* findPlane(f32 height, i32 picnum) {
	visplane_t* check;
	for (check = visplanes; check < lastvisplane; check++) {
		if (height == check->height && picnum == check->picnum) {
			break;
		}
	}
	if (check < lastvisplane) return check;

	if (lastvisplane - visplanes == MAXVISPLANES) return NULL;

	lastvisplane++;

	check->height = height;
	check->picnum = picnum;
	check->minx = SCREEN_WIDTH;
	check->maxx = -1;

	for (int i = 0; i < SCREEN_WIDTH; i++) {
		check->top[i] = SCREEN_HEIGHT + 1;
	}

	return check;
}

visplane_t* checkPlane(visplane_t* v, i32 start, i32 stop) {
	i32 intrl, intrh, unionl, unionh, x;
	if (start < v->minx) {
		intrl = v->minx + 1;
		unionl = start;
	}
	else {
		unionl = v->minx;
		intrl = start;
	}

	if (stop > v->maxx) {
		intrh = v->maxx + 1;
		unionh = stop;
	}
	else {
		unionh = v->maxx;
		intrh = stop;
	}
	
	for (x = intrl; x <= intrh; x++) {
		if (v->top[x] != SCREEN_HEIGHT + 1) break;
	}

	if (x >= intrh) {
		v->minx = unionl;
		v->maxx = unionh;
		return v;
	}
	if (lastvisplane - visplanes == MAXVISPLANES) return NULL;

	lastvisplane->height = v->height;
	lastvisplane->picnum = v->picnum;

	v = lastvisplane++;
	v->minx = start;
	v->maxx = stop;

	for (int i = 0; i < SCREEN_WIDTH; i++) {
		v->top[i] = SCREEN_HEIGHT + 1;
	}

	return v;
}

void drawPlanes3D(Player player, Map* map, u32* pixels, Texture* tex) {
	u32 colors[8] = { BLUE,RED,GREEN,YELLOW,PURPLE,ORANGE,WHITE,LIGHTGRAY };
	visplane_t* v;
	u32 color = 0;
	for (v = visplanes; v < lastvisplane; v++) {
		color++;
		for (i32 x = v->minx; x < v->maxx; x++) {
			for (i32 y = v->bottom[x]; y < v->top[x]; y++) {
				if (v->top[x] == SCREEN_HEIGHT + 1) continue;
				f32 a = screenxtoangle[x];
				//scale normalized yslope by actual camera pos and divide by the cosine of angle to prevent fisheye effect
				f32 dis = fabs(((player.pos.z - v->height) * yslope[y]) / (cos(a)));
				//relative coordinates to player
				f32 xt = -sin(a) * dis;
				f32 yt = cos(a) * dis;
				// absolute coordinates
				v2 p = camera_pos_to_world((v2) { xt, yt }, player);
				// texutre coordinates
				f32 pixelshade = calcFlatShade(dis);
				v2i t = { abs((i32)(fmod((p.x), 8.0f) * 32.0f)) ,abs((i32)(fmod((p.y), 8.0f) * 32.0f)) };
				u32 color = changeRGBBrightness(tex[0].pixels[(256 - t.y) * tex[0].width + t.x], pixelshade);
				drawPixel(x, y, color, pixels);
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}
		}
	}
}

void drawSprites(Player player, Map* map, u32* pixels, Texture* tex) {
	//TODO: PROPER ENTITYHANDER AND ENTITY
	v2 spriteScale = { 7.5f, 7.5f };
	f32 spritez = 6.0f;

	f32 spritevMove = -SCREEN_HEIGHT / 2 * (spritez - player.pos.z);
	v2 spritepos = { 18.0f ,18.0f };
	v2 spritep = world_pos_to_camera(spritepos, player);
	v2 test = camera_pos_to_world(spritep, player);
	f32 spritea = atan2(spritep.y, spritep.x) - PI / 2;
	i32 spriteScreenX = screen_angle_to_x(spritea);
	i32 vMoveScreen = spritevMove / spritep.y;

	//calculate camera spriteheight and textureheight
	i32 spriteHeight = (SCREEN_HEIGHT / spritep.y) * spriteScale.y;
	i32 y0 = -spriteHeight / 2 + SCREEN_HEIGHT / 2 + vMoveScreen;
	if (y0 < 0) y0 = 0;
	i32 y1 = spriteHeight / 2 + SCREEN_HEIGHT / 2 + vMoveScreen;
	if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT - 1;

	i32 spriteWidth = (SCREEN_HEIGHT / spritep.y) * spriteScale.x;
	i32 x0 = -spriteWidth / 2 + spriteScreenX;
	if (x0 < 0) x0 = 0;
	i32 x1 = spriteWidth / 2 + spriteScreenX;
	if (x1 >= SCREEN_WIDTH) x1 = SCREEN_WIDTH - 1;

	Texture sprite = tex[1];

	for (i32 x = x0; x < x1; x++) {
		v2i tex;
		tex.x = (i32)((x - (-spriteWidth / 2 + spriteScreenX)) * sprite.width / spriteWidth);
		if (spritep.y > 0 && x >= 0 && x < SCREEN_WIDTH) {
			for (i32 y = y0; y < y1; y++) {
				if (zBuffer[((SCREEN_HEIGHT - 1) - y) * SCREEN_WIDTH + x] > spritep.y) {
					i32 d = (y - vMoveScreen) - SCREEN_HEIGHT / 2 + spriteHeight / 2;
					tex.y = (d * sprite.height) / spriteHeight;
					u32 color = changeRGBBrightness(sprite.pixels[tex.y * sprite.width + tex.x], calcFlatShade(spritep.y));
					drawPixel(x, SCREEN_HEIGHT - y, color, pixels);
				}
			}
		}
	}
}

void drawMinimap(Player player, Map* map, u32* pixels) {
	v2i mapoffset = (v2i){ 100 , SCREEN_HEIGHT - 200 };

	for (i32 j = 0; j < map->sectornum; j++)
	{
		Sector sec = map->sectors[j];
		for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {
			Wall w = map->walls[i];
			drawLine(w.a.x + mapoffset.x, w.a.y + mapoffset.y, w.b.x + mapoffset.x, w.b.y + mapoffset.y, WHITE, pixels);
		}
	}
	drawCircle(player.pos.x + mapoffset.x, player.pos.y + mapoffset.y, 3, 3, WHITE, pixels);
	drawLine(player.pos.x + mapoffset.x, player.pos.y + mapoffset.y, player.anglecos * 10 + player.pos.x + mapoffset.x, player.anglesin * 10 + player.pos.y + mapoffset.y, WHITE, pixels);
}