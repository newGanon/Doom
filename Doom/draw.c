#include "draw.h"
#include "math.h"
#include "map.h"

typedef struct WallRenderingInfo {
	int sectorno, sx1, sx2;
	u8 renderedSectors[SECTOR_MAX];
} WallRenderingInfo;

typedef struct visplane_t {
	f32	height;
	i32	picnum;
	i32	lightlevel;
	i32	minx;
	i32	maxx;
	u16 top[SCREEN_WIDTH];
	u16 bottom[SCREEN_WIDTH];

} visplane_t;

void drawPixel(i32 x, i32 y, u32 color);
void drawTexLine(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, f64 u, Texture* tex, f32 shade, f32 dis, f32 wallheight, f32 wallwidth, Wall* wall, u8 portalbottom);
void drawWall3D(Player player, Texture* tex, WallRenderingInfo* now, u32 rd);
void drawPlanes3D(Player player, Texture* tex);
void makeSpans(i32 x, i32 t1, i32 b1, i32 t2, i32 b2, visplane_t* v, Player player, Texture* tex);
void mapPlane(i32 y, i32 x1, i32 x2, visplane_t* v, Player player, Texture* tex);
void drawMinimap(Player player);
void clearPlanes();
void drawSprites(Player player, Texture* tex, EntityHandler* h);
void drawVerticalLine(i32 x, i32 y0, i32 y1, u32 color);
void drawLine(i32 x0, i32 y0, i32 x1, i32 y1, u32 color);
void drawSquare(i32 x0, i32 y0, u32 size, u32 color);
void fillSquare(i32 x0, i32 y0, u32 size, u32 color);
void fillRectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color);
void drawCircle(i32 x0, i32 y0, i32 a, i32 b, u32 color);

visplane_t* findPlane(f32 height, i32 picnum);
visplane_t* checkPlane(visplane_t* v, i32 start, i32 stop);

u32 changeRGBBrightness(u32 color, f32 factor);
f32 calcWallShade(v2 start, v2 end, f32 dis);
f32 calcFlatShade(f32 dis);

u8 is_transparent(u32 color);

visplane_t visplanes[MAXVISPLANES];
visplane_t* lastvisplane;
visplane_t* floorplane;
visplane_t* ceilplane;

v2  zdl, zdr, znl, znr, zfl, zfr;

u32* pixels;

//lockuptable of how far you would need to travel in y direction to move 1 in horizontal direction for each y value
f32 yslope[SCREEN_HEIGHT];
//lockuptable of pixel x value to angle
f32 screenxtoangle[SCREEN_WIDTH];

f32 zBuffer[SCREEN_HEIGHT * SCREEN_WIDTH];

//clipping information of walls
u16 ceilingclip[SCREEN_WIDTH * SECTOR_MAX];
u16 floorclip[SCREEN_WIDTH * SECTOR_MAX];

//used for horizontal drawing of visplain strips
i32 spanstart[SCREEN_HEIGHT];


void drawPixel(i32 x, i32 y, u32 color) {
	if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && !is_transparent(color)) {
		pixels[y * SCREEN_WIDTH + x] = color;
	}
}

void drawInit(u32* pixels1) {
	// init global render variables 
	pixels = pixels1;

	// yslope if camera had z position 1, this value is later scaled by the actual height value
	for (i32 y = 0; y < SCREEN_HEIGHT; y++) {
		f32 dy = y - SCREEN_HEIGHT / 2.0f;
		yslope[y] = (SCREEN_HEIGHT * VFOV) / dy;
	}
	yslope[SCREEN_HEIGHT / 2] = 1000;

	for (i32 x = 0; x < SCREEN_WIDTH; x++) {
		screenxtoangle[x] = screen_x_to_angle(x);
	}

	// Set wall drawing variables that don't change
	zdl = v2Rotate(((v2) { 0.0f, 1.0f }), +(HFOV / 2.0f)),
	zdr = v2Rotate(((v2) { 0.0f, 1.0f }), -(HFOV / 2.0f)),
	znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR },
	znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR },
	zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR },
	zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR };

	// calculate a color LUT for fast shading of a specific color;

}

void draw3D(Player player, Texture* tex, EntityHandler* h) {

	clearPlanes();

	drawWall3D(player, tex, &(WallRenderingInfo) { player.sector, 0, SCREEN_WIDTH - 1, { 0 }}, 0);

	drawPlanes3D(player, tex);

	drawSprites(player, tex, h);

	drawMinimap(player);
}

void drawVerticalLine(i32 x, i32 y0, i32 y1, u32 color) {
	if (y0 > y1) {
		i32 temp = y0;
		y0 = y1;
		y1 = temp;
	}
	for (i32 y = y0; y <= y1; y++) {
		drawPixel(x, y, color);
	}
}

/* Bresenham's line algorithm */
void drawLine(i32 x0, i32 y0, i32 x1, i32 y1, u32 color) {
	i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	i32 dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	i32 err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		drawPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 > -dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void drawSquare(i32 x0, i32 y0, u32 size, u32 color) {
	//right and left line
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		drawPixel(x0, y, color);
		drawPixel(x0 + size, y, color);
	}
	//top and bottom line
	for (i32 x = x0; x < (x0 + (i32)size); x++) {
		drawPixel(x, y0, color);
		drawPixel(x, y0 + size, color);
	}
}

void fillSquare(i32 x0, i32 y0, u32 size, u32 color) {
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		for (i32 x = x0; x < (x0 + (i32)size); x++) {
			drawPixel(x, y, color);
		}
	}
}

void fillRectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color) {
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
			drawPixel(x, y, color);
		}
	}
}

/* Bresenham's circle algorithm */
void drawCircle(i32 x0, i32 y0, i32 a, i32 b, u32 color) {
	i32 dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
	i64 a2 = a * a, b2 = b * b;
	i64 err = b2 - (2 * b - 1) * a2, e2; /* Fehler im 1. Schritt */
	do {
		drawPixel(x0 + dx, y0 + dy, color);
		drawPixel(x0 + dx, y0 - dy, color);
		drawPixel(x0 - dx, y0 - dy, color);
		drawPixel(x0 - dx, y0 + dy, color);
		e2 = 2 * err;
		if (e2 < (2 * dx + 1) * b2) { ++dx; err += (2 * dx + 1) * b2; }
		if (e2 > -(2 * dy - 1) * a2) { --dy; err -= (2 * dy - 1) * a2; }
	} while (dy >= 0);
	/* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
	while (dx++ < a) {
		/* Spitze der Ellipse vollenden */
		drawPixel(x0 + dx, y0, color);
		drawPixel(x0 - dx, y0, color);
	}
}

//slow, dont use
u32 changeRGBBrightness(u32 color, f32 factor) {
	//return color;
	i32 a = (color & 0xFF000000);
	i32 r = (color & 0x00FF0000) >> 16;
	i32 g = (color & 0x0000FF00) >> 8;
	i32 b = color & 0x000000FF;
	return a | (i32)(r / factor) << 16 | (i32)(g / factor) << 8 | (i32)(b / factor);
}

f32 calcWallShade(v2 start, v2 end, f32 dis) {
	v2 difNorm = v2Normalize(v2Sub(end, start));
	return (f32)1.0f + (10.0f * (fabsf(difNorm.x)) + fabsf(dis)) * LIGHTDIMINISHINGDFACTOR;
}

f32 calcFlatShade(f32 dis) {
	return (f32)1 + fabsf(dis) * LIGHTDIMINISHINGDFACTOR;
}


void drawWall3D(Player player, Texture* tex, WallRenderingInfo* now, u32 rd)
{
	if (rd > 32) return;
	Sector sec = *get_sector(now->sectorno-1);
	for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {

		Wall w = *get_wall(i);
		//world pos
		v2 p1 = world_pos_to_camera(w.a, player);
		v2 p2 = world_pos_to_camera(w.b, player);

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
		if (a1 < a2 || a2 < -(HFOV / 2) - 0.001f || a1 > +(HFOV / 2) + 0.001f) continue;

		//convert the angle of the wall into screen coordinates (player FOV is 90 degrees or 1/2 PI)
		i32 tx1 = screen_angle_to_x(a1);
		i32 tx2 = screen_angle_to_x(a2);
		if (tx1 != 0) tx1++;

		if (tx1 > now->sx2) continue;
		if (tx2 < now->sx1) continue;

		i32 x1 = clamp(tx1, now->sx1, now->sx2);
		i32 x2 = clamp(tx2, now->sx1, now->sx2);

		if (x1 >= x2) continue;

		//rempove part of wall that is already covered by wall, works because we sort walls from near to far, only happens in non konvex rooms
		for (i32 i = x1; i < x2; i++) {
			if (ceilingclip[i] == 0) {
				x1 = i+1;
			}
			else break;
		}

		for (i32 i = x2; i > x1; i--) {
			if (ceilingclip[i] == 0) {
				x2 = i-1;
			}
			else break;
		}
		
		floorplane = findPlane(sec.zfloor, 0);
		ceilplane = findPlane(sec.zceil, 0);

		floorplane = checkPlane(floorplane, x1, x2);
		ceilplane = checkPlane(ceilplane, x1, x2);

		//get floor and ceiling height of sector behind wall if wall is a portal
		f32 nzfloor = sec.zfloor;
		f32 nzceil = sec.zceil;

		if (w.portal != 0) {
			Sector portal = *get_sector(w.portal-1);
			nzfloor = portal.zfloor;
			nzceil = portal.zceil;
		}

		f32 sy0 = (VFOV * SCREEN_HEIGHT) / p1.y;
		f32 sy1 = (VFOV * SCREEN_HEIGHT) / p2.y;

		//wall coordinates
		i32 yf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player.z) * sy0);
		i32 yf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player.z) * sy1);
		i32 yc0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player.z) * sy0);
		i32 yc1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player.z) * sy1);
		//portal coordinates in the wall
		i32 pf0 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player.z) * sy0);
		i32 pf1 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player.z) * sy1);
		i32 pc0 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player.z) * sy0);
		i32 pc1 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player.z) * sy1);

		//wall texture mapping varaibles
		v2 difp1 = v2Sub(p1, tp1);
		v2 difp2 = v2Sub(p2, tp2);
		f32 twlen = v2Len(v2Sub(tp1, tp2));
		v2 cutoff = { fabsf(v2Len(difp1) / twlen), fabsf(v2Len(difp2) / twlen) };

		for (i32 x = x1; x <= x2; x++) {
			//calculate x stepsize
			f32 xp = (x - tx1) / (f32)(tx2 - tx1);

			//get top and bottom coordinates of the wall
			i32 tyf = (i32)(xp * (yf1 - yf0)) + yf0;
			i32 tyc = (i32)(xp * (yc1 - yc0)) + yc0;
			i32 yf = clamp(tyf, floorclip[x], ceilingclip[x]);
			i32 yc = clamp(tyc, floorclip[x], ceilingclip[x]);
			
			//set visplane top and bottom clipping
			//TODO: DRAW PLANES 1 PIXEL ON TOP AND BOTTOM HIGHER
			if (yc < ceilingclip[x]) {
				ceilplane->bottom[x] = yc;
				ceilplane->top[x] = ceilingclip[x];
			}
			if (yf > floorclip[x]) {
				floorplane->bottom[x] = floorclip[x];
				floorplane->top[x] = yf;
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

			//TODO PROPER LIGHTING
			f32 wallshade = calcWallShade(w.a, w.b, dis);
			//f32 wallshade = 1;

			f32 wallheight;

			f32 dx = w.a.x - w.b.x;
			f32 dy = w.a.y - w.b.y;
			f32 wallwidth = sqrt(dx * dx + dy * dy);

			//draw Wall
			if (w.portal == 0) {
				//drawVerticalLine(x, yf, yc, changeRGBBrightness(color, wallshade), pixels); wall in one color
				if (yc > tyf && yf < tyc) {
					f32 wallheight = sec.zceil - sec.zfloor;
					drawTexLine(x, yf, yc, tyf, tyc, u, tex, wallshade, dis, wallheight, wallwidth, &w, 0);
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
				if (pyf > yf) { 
					wallheight = nzfloor - sec.zfloor;
					drawTexLine(x, yf, pyf, tyf, tpyf, u, tex, wallshade, dis, wallheight, wallwidth, &w, 0);
				}
				//draw window
				//drawVerticalLine(x, pyf, pyc, color, pixels);
				//if neighborceiling is lower than current sectorceiling then draw it
				if (pyc < yc) { 
					wallheight = sec.zceil - nzceil;
					drawTexLine(x, pyc, yc, tpyc, tyc, u, tex, wallshade, dis, wallheight, wallwidth, &w, 0);
				}

				//update vertical clipping arrays
				ceilingclip[x] = clamp(pyc, 0, ceilingclip[x]);
				floorclip[x] = clamp(pyf, floorclip[x], SCREEN_HEIGHT - 1);
			}
		}

		if (w.portal && !now->renderedSectors[w.portal - 1]) {
			WallRenderingInfo* wr = &(WallRenderingInfo){ w.portal, x1, x2};
			memcpy(wr->renderedSectors, now->renderedSectors, SECTOR_MAX * sizeof(u8));
			wr->renderedSectors[now->sectorno - 1] = 1;
			drawWall3D(player, tex, wr, ++rd);
		}
	}
}

void drawTexLine(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, f64 u, Texture* tex, f32 shade, f32 dis, f32 wallheight, f32 wallwidth, Wall* wall, u8 portalbottom) {
	// draw decals
	f32 wall_pos_x = u * wallwidth;
	bool decal[SCREEN_HEIGHT] = { false };
	for (Decal* d = wall->decalhead; d != NULL; d = d->next) {
		// decal not in current stripe of wall, check next decal
		if (!(d->wallpos.x < wall_pos_x && (d->wallpos.x + d->size.x) > wall_pos_x)) continue;
		f32 decal_pos_x = wall_pos_x - d->wallpos.x;
		f32 decal_tx = (decal_pos_x / d->size.x) * d->tex->width;

		f32 decal_top_y = (d->wallpos.y + d->size.y) / wallheight;
		f32 decal_bot_y = d->wallpos.y / wallheight;
		i32 decal_top_ty = decal_top_y * (yc - yf) + yf;
		i32 decal_bot_ty = decal_bot_y * (yc - yf) + yf;

		f32 decal_step = (d->tex->height) / (f32)(decal_top_ty - decal_bot_ty);
		decal_top_ty = clamp(decal_top_ty, y0, y1);
		f32 decal_ty = 0;
		if (decal_bot_ty < y0) {
			decal_ty = (y0 - decal_bot_ty) * decal_step;
		}

		decal_bot_ty = clamp(decal_bot_ty, y0, y1);

		for (i32 y = decal_bot_ty; y < decal_top_ty; y++) {
			// only draw pixel of decal if pixel has not already been drawn by other decal
			if (!decal[y]) {
				u32 color = d->tex->pixels[((i32)((d->tex->height - 1) - decal_ty)) * d->tex->height + (i32)decal_tx];
				if (!is_transparent(color)) {
					color = changeRGBBrightness(color, shade);
					decal[y] = true;
					drawPixel(x, y, color);
					zBuffer[y * SCREEN_WIDTH + x] = dis;
				}
			}
			decal_ty += decal_step;
		}
	}

	// draw walls
	f32 texture_scale = 4.0f;
	f32 texheight = wallheight / texture_scale;
	f32 texwidth = wallwidth / texture_scale;
	v2i walloffset;

	walloffset.x = (u * texwidth) * tex[0].height;

	i32 tx = walloffset.x % tex[0].width;

	f32 texy0 = ((1.0 - ((y0 - yf) / (f64)(yc - yf)))) * tex->height * texheight;
	f32 texy1 = ((1.0 - ((y1 - yf) / (f64)(yc - yf)))) * tex->height * texheight;
	f32 stepy = ((texy1 - texy0) / (y1 - y0));

	for (i32 y = y0; y <= y1; y++) {
		// only draw wall when there is no decal
		if (!decal[y]) {
			walloffset.y = texy0;
			i32 ty = walloffset.y % tex[0].height;
			u32 color = changeRGBBrightness(tex[0].pixels[ty * tex[0].width + tx], shade);
			drawPixel(x, y, color);
			zBuffer[y * SCREEN_WIDTH + x] = dis;
		}
		texy0 += stepy;
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
	u8 getNewest = 1;
	visplane_t* check;

	//reversed doom algorithm
	if (getNewest) {
		for (check = lastvisplane - 1; check >= visplanes; check--) {
			if (height == check->height && picnum == check->picnum) {
				break;
			}
		}
		//if (check < lastvisplane) return check;
		if (check >= visplanes) return check;

		check = lastvisplane;
	}
	//like doom used to do it
	else {
		for (check = visplanes; check < lastvisplane; check++) {
			if (height == check->height && picnum == check->picnum) {
				break;
			}
		}
		//if (check < lastvisplane) return check;
		if (check < lastvisplane) return check;
	}

	if (lastvisplane - visplanes == MAXVISPLANES) return NULL;

	lastvisplane++;

	check->height = height;
	check->picnum = picnum;
	check->minx = SCREEN_WIDTH - 1;
	check->maxx = -1;

	for (int i = 0; i < SCREEN_WIDTH; i++) {
		check->bottom[i] = SCREEN_HEIGHT + 1;
		check->top[i] = 0;
	}

	return check;
}

visplane_t* checkPlane(visplane_t* v, i32 start, i32 stop) {
	i32 intrl, intrh, unionl, unionh, x;
	if (start < v->minx) {
		intrl = v->minx;
		unionl = start;
	}
	else {
		unionl = v->minx;
		intrl = start;
	}

	if (stop > v->maxx) {
		intrh = v->maxx;
		unionh = stop;
	}
	else {
		unionh = v->maxx;
		intrh = stop;
	}
	
	for (x = intrl; x <= intrh; x++) {
		if (v->bottom[x] != SCREEN_HEIGHT + 1) break;
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
		v->bottom[i] = SCREEN_HEIGHT + 1;
		v->top[i] = 0;
	}

	return v;
}

void makeSpans(i32 x, i32 t1, i32 b1, i32 t2, i32 b2, visplane_t* v, Player player, Texture* tex) {
	while (t1 > t2 && t1 >= b1) {
		mapPlane(t1, spanstart[t1], x+1, v, player, tex);
		t1--;
	}
	while (b1 < b2 && b1 <= t1) {
		mapPlane(b1, spanstart[b1], x+1, v, player, tex);
		b1++;
	}

	while (t2 > t1 && t2 >= b2) {
		spanstart[t2] = x;
		t2--;
	}
	while (b2 < b1 && b2 <= t2) {
		spanstart[b2] = x;
		b2++;
	}
}

void mapPlane(i32 y, i32 x1, i32 x2, visplane_t* v, Player player, Texture* tex) {
	f32 tex_scale = 4.0f;

	i32 texheight = 256;
	i32 texwidth = 256;

	v2 texsizefactor = { texwidth / tex_scale, texheight / tex_scale };

	f32 a = screenxtoangle[x1];

	//other implementation
	//f32 dis = fabs(((player.pos.z - v->height) * (SCREEN_WIDTH / 2) * yslope[y]));
	// absolute coordinates
	//v2 p = {player.pos.x * (SCREEN_WIDTH / 2) + (dis/cos(a)) * cos(player.angle + a),  player.pos.y * (SCREEN_WIDTH / 2) + (dis / cos(a)) * sin(player.angle + a)};

	//scale normalized yslope by actual camera pos and divide by the cosine of angle to prevent fisheye effect
	f32 dis = fabs(((player.z - v->height) * yslope[y]));
	//relative coordinates to player
	f32 xt = -sin(a) * dis / cos(a);
	f32 yt = cos(a) * dis / cos(a);
	// absolute coordinates
	v2 p = camera_pos_to_world((v2) { xt, yt }, player);

	p.x *= texsizefactor.x;
	p.y *= texsizefactor.y;

	f32 step = dis / (SCREEN_WIDTH / 2);

	f32 xstep = step * cos(player.angle - PI_2) * texsizefactor.x;
	f32 ystep = step * sin(player.angle - PI_2) * texsizefactor.y;

	//TODO PRPOPER LIGHTING
	f32 pixelshade = calcFlatShade(dis);
	//f32 pixelshade = 1.0f;

	for (i32 x = x1; x <= x2; x++)
	{
		v2i t = { (i32)(p.x) & (texwidth - 1), (i32)(p.y) & (texwidth - 1) };

		u32 color = changeRGBBrightness(tex[0].pixels[(texheight - 1 - t.y) * tex[0].width + t.x], pixelshade);
		drawPixel(x, y, color);
		zBuffer[y * SCREEN_WIDTH + x] = dis;
		p.x += xstep;
		p.y += ystep;
	}
}

void drawPlanes3D(Player player, Texture* tex) {
	
	for (visplane_t* v = visplanes; v < lastvisplane; v++) {
		if (v->minx > v->maxx) continue;
		
		v->top[v->maxx] = 0;
		v->top[v->minx] = 0;

		for (i32 x = v->minx ; x < v->maxx; x++)
		{
			makeSpans(x, v->top[x], v->bottom[x], v->top[x+1], v->bottom[x+1], v, player, tex);
		}
	}


	/*u32 colors[8] = {BLUE,RED,GREEN,YELLOW,PURPLE,ORANGE,WHITE,LIGHTGRAY};
	visplane_t* v;
	u32 color = 0;
	i32 texheight = 256;
	i32 texwidth = 256;
	for (v = visplanes; v < lastvisplane; v++) {
		color++;
		for (i32 x = v->minx; x <= v->maxx; x++) {
			for (i32 y = v->bottom[x]; y < v->top[x]; y++) {
				f32 a = screenxtoangle[x];
				//scale normalized yslope by actual camera pos and divide by the cosine of angle to prevent fisheye effect
				f32 dis = fabs(((player.pos.z - v->height) * yslope[y]));
				//relative coordinates to player
				f32 xt = -sin(a) * dis / (cos(a));
				f32 yt = cos(a) * dis / (cos(a));
				// absolute coordinates
				v2 p = camera_pos_to_world((v2) { xt, yt }, player);
				// texutre coordinates
				f32 pixelshade = calcFlatShade(dis);
				v2i t = { (i32)((p.x - ((i32)p.x)) * texheight) & (texwidth - 1), (i32)((p.y - ((i32)p.y)) * texheight) & (texwidth - 1)};
				//u32 color = changeRGBBrightness(tex[0].pixels[(256 - t.y) * tex[0].width + t.x], pixelshade);
				drawPixel(x, y, colors[color%8], pixels);
				//drawPixel(x, y, color, pixels);
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}
		}
	}*/
}


void drawSprites(Player player, Texture* tex, EntityHandler* h) {

	for (i32 i = 0; i < h->used; i++)
	{
		Entity e = *h->entities[i];
		if (e.type == Projectile){
			f32 dx = e.pos.x - player.pos.x;
			f32 dy = e.pos.y - player.pos.y;
			f32 dis = sqrt(dx * dx + dy * dy);
			if (dis < 4.0f) {
				continue;
			}
		}

		if (e.relCamPos.y <= 0) continue;

		f32 spritevMove = -SCREEN_HEIGHT  * (e.z - player.z);
		f32 spritea = atan2(e.relCamPos.y, e.relCamPos.x) - PI / 2;
		i32 spriteScreenX = screen_angle_to_x(spritea);
		i32 vMoveScreen = spritevMove / e.relCamPos.y;

		//calculate camera spriteheight and textureheight
		i32 spriteHeight = (SCREEN_HEIGHT / e.relCamPos.y) * e.scale.y;
		if (spriteHeight == 0) continue;
		i32 y0 = -spriteHeight / 2 + SCREEN_HEIGHT / 2 + vMoveScreen;
		if (y0 < 0) y0 = 0;
		i32 y1 = spriteHeight / 2 + SCREEN_HEIGHT / 2 + vMoveScreen;
		if (y1 >= SCREEN_HEIGHT) y1 = SCREEN_HEIGHT;

		i32 spriteWidth = (SCREEN_HEIGHT / e.relCamPos.y) * e.scale.x;
		i32 x0 = -spriteWidth / 2 + spriteScreenX;
		if (x0 < 0) x0 = 0;
		i32 x1 = spriteWidth / 2 + spriteScreenX;
		if (x1 >= SCREEN_WIDTH) x1 = SCREEN_WIDTH;

		Texture sprite = tex[e.spriteNum[0]];

		//calc tex pos and tex steps
		f32 texy0 = ((y0 - vMoveScreen - SCREEN_HEIGHT / 2 + spriteHeight / 2) * sprite.height) / spriteHeight;
		f32 texy1 = (((y1)-vMoveScreen - SCREEN_HEIGHT / 2 + spriteHeight / 2) * sprite.height) / spriteHeight;
		f32 ystep = (texy1 - texy0) / (y1 - y0);

		f32 texx0 = (i32)((x0 - (-spriteWidth / 2 + spriteScreenX)) * sprite.width / spriteWidth);
		f32 texx1 = (i32)((x1 - (-spriteWidth / 2 + spriteScreenX)) * sprite.width / spriteWidth);
		f32 xstep = (texx1 - texx0) / (x1 - x0);

		v2 texpos;
		f32 pixelshade = calcFlatShade(e.relCamPos.y);
		texpos.y = texy0;
		for (i32 y = y0; y < y1; y++) {
			texpos.y += ystep;
			texpos.x = texx0;
			for (i32 x = x0; x < x1; x++) {
				i32 index = ((SCREEN_HEIGHT - 1) - y) * SCREEN_WIDTH + x;
				if (zBuffer[index] > e.relCamPos.y) {
					u32 color = changeRGBBrightness(sprite.pixels[(i32)texpos.y * sprite.width + (i32)texpos.x], pixelshade);
					drawPixel(x, SCREEN_HEIGHT - y, color);
					if ((color & 0xFF000000) != 0) zBuffer[index] = e.relCamPos.y;
				}
				texpos.x += xstep;
			}
		}
	}
}

void drawMinimap(Player player) {
	v2i mapoffset = (v2i){ 100 , SCREEN_HEIGHT - 200 };

	i32 sectornum = get_sectornum();
	for (i32 j = 0; j < sectornum; j++)
	{
		Sector sec = *get_sector(j);
		for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {
			Wall w = *get_wall(i);
			drawLine(w.a.x + mapoffset.x, w.a.y + mapoffset.y, w.b.x + mapoffset.x, w.b.y + mapoffset.y, WHITE);
		}
	}
	drawCircle(player.pos.x + mapoffset.x, player.pos.y + mapoffset.y, 3, 3, WHITE);
	drawLine(player.pos.x + mapoffset.x, player.pos.y + mapoffset.y, player.anglecos * 10 + player.pos.x + mapoffset.x, player.anglesin * 10 + player.pos.y + mapoffset.y, WHITE);
}


u8 is_transparent(u32 color) {
	return (color & 0xFF000000) == 0;
}