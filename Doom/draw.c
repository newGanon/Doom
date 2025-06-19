#include "draw.h"
#include "math.h"
#include "map.h"
#include "player.h"
#include "entityhandler.h"
#include "entity.h"
#include "tex.h"


//TODO: make queue for transparent wall sliced, that should get drawn when everyhtning has finished drawing

typedef struct WallRenderingInfo {
	int sectorno, sx1, sx2;
	bool renderedSectors[SECTOR_MAX];
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


void inline draw_pixel(i32 x, i32 y, u32 color);
void inline draw_pixel_from_lightmap(i32 x, i32 y, u8 index, u8 shade);
void draw_tex_line(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, i32 ayf, f32 u, u8 shade_index, f32 dis, f32 zfloor, f32 zfloor_old, f32 wallheight, f32 wallwidth, Wall* wall, wall_section_type type, bool wall_backside);
void draw_wall_3d(Player* player, WallRenderingInfo* now, u32 rd);
void draw_transparent_walls(Player* player);
void draw_planes_3d(Player* player);
void draw_make_spans(i32 x, i32 t1, i32 b1, i32 t2, i32 b2, visplane_t* v, Player* player);
void draw_map_plane(i32 y, i32 x1, i32 x2, visplane_t* v, Player* player);
void draw_minimap(Player* player);
void draw_clear_planes();
void draw_sprites(Player* player, EntityHandler* handler);
void draw_vertical_line(i32 x, i32 y0, i32 y1, u32 color);
void draw_line(i32 x0, i32 y0, i32 x1, i32 y1, u32 color);
void draw_square(i32 x0, i32 y0, u32 size, u32 color);
void draw_fill_square(i32 x0, i32 y0, u32 size, u32 color);
void draw_fill_rectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color);
void draw_circle(i32 x0, i32 y0, i32 a, i32 b, u32 color);

visplane_t* draw_find_plane(f32 height, i32 picnum);
visplane_t* draw_check_plane(visplane_t* v, i32 start, i32 stop);

// used for everything that is not a wall
v3 calc_tex_start_and_step(i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale);
// used for portal walls
v3 calc_tex_low_high_and_step(i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale);
// used for walls
v2 calc_tex_start_and_step_abs(i32 true_abs_low, i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale);

static visplane_t visplanes[MAXVISPLANES];
static visplane_t* lastvisplane;
static visplane_t* floorplane;
static visplane_t* ceilplane;

static v2 zdl, zdr, znl, znr, zfl, zfr;

// global variables
static u32* pixels;
static Palette* lightmap;
static LightmapindexTexture* index_textures;

//lockuptable of how far you would need to travel in y direction to move 1 in horizontal direction for each y value
static f32 yslope[SCREEN_HEIGHT];
//lockuptable of pixel x value to angle
static f32 screenxtoangle[SCREEN_WIDTH];

static f32 zBuffer[SCREEN_HEIGHT * SCREEN_WIDTH];

//clipping information of walls
static u16 ceilingclip[SCREEN_WIDTH];
static u16 floorclip[SCREEN_WIDTH];

//used for horizontal drawing of visplain strips
static i32 spanstart[SCREEN_HEIGHT];

static bool drawn_sectors[SECTOR_MAX];


void inline draw_pixel(i32 x, i32 y, u32 color) {
	//if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT && !is_transparent(color)) {
	//	pixels[y * SCREEN_WIDTH + x] = color;
	//}
	pixels[y * SCREEN_WIDTH + x] = color;
}

void inline draw_pixel_from_lightmap(i32 x, i32 y, u8 index, u8 shade) {
	pixels[y * SCREEN_WIDTH + x] = lightmap->colors[index][shade];
}

void draw_init(u32* pixels1, Palette* lightmap1, LightmapindexTexture* index_textures1) {
	// init global render variables 
	pixels = pixels1;
	lightmap = lightmap1;
	index_textures = index_textures1;

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
	zdl = v2_rotate(((v2) { 0.0f, 1.0f }), +(HFOV / 2.0f)),
	zdr = v2_rotate(((v2) { 0.0f, 1.0f }), -(HFOV / 2.0f)),
	znl = (v2){ zdl.x * ZNEAR, zdl.y * ZNEAR },
	znr = (v2){ zdr.x * ZNEAR, zdr.y * ZNEAR },
	zfl = (v2){ zdl.x * ZFAR, zdl.y * ZFAR },
	zfr = (v2){ zdr.x * ZFAR, zdr.y * ZFAR };
}

void draw_3d(Player* player, EntityHandler* handler) {
	draw_clear_planes();
	WallRenderingInfo wr = { player->sector, 0, SCREEN_WIDTH - 1, { 0 } };
	wr.renderedSectors[player->sector] = true;
	memset(drawn_sectors, 0, sizeof(drawn_sectors));
	draw_wall_3d(player, &wr, 0);
	draw_planes_3d(player);
	draw_transparent_walls(player);
	draw_sprites(player, handler);
	draw_minimap(player);
}

void draw_vertical_line(i32 x, i32 y0, i32 y1, u32 color) {
	if (y0 > y1) {
		i32 temp = y0;
		y0 = y1;
		y1 = temp;
	}
	for (i32 y = y0; y <= y1; y++) {
		draw_pixel(x, y, color);
	}
}

/* Bresenham's line algorithm */
void draw_line(i32 x0, i32 y0, i32 x1, i32 y1, u32 color) {
	i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	i32 dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	i32 err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		draw_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 > -dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void draw_square(i32 x0, i32 y0, u32 size, u32 color) {
	//right and left line
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		draw_pixel(x0, y, color);
		draw_pixel(x0 + size, y, color);
	}
	//top and bottom line
	for (i32 x = x0; x < (x0 + (i32)size); x++) {
		draw_pixel(x, y0, color);
		draw_pixel(x, y0 + size, color);
	}
}

void draw_fill_square(i32 x0, i32 y0, u32 size, u32 color) {
	for (i32 y = y0; y < (y0 + (i32)size); y++) {
		for (i32 x = x0; x < (x0 + (i32)size); x++) {
			draw_pixel(x, y, color);
		}
	}
}

void draw_fill_rectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color) {
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
			draw_pixel(x, y, color);
		}
	}
}

/* Bresenham's circle algorithm */
void draw_circle(i32 x0, i32 y0, i32 a, i32 b, u32 color) {
	i32 dx = 0, dy = b; /* im I. Quadranten von links oben nach rechts unten */
	i64 a2 = (i64)a * (i64)a, b2 = (i64)b * (i64)b;
	i64 err = b2 - (2 * (i64)b - 1) * a2, e2; /* Fehler im 1. Schritt */
	do {
		draw_pixel(x0 + dx, y0 + dy, color);
		draw_pixel(x0 + dx, y0 - dy, color);
		draw_pixel(x0 - dx, y0 - dy, color);
		draw_pixel(x0 - dx, y0 + dy, color);
		e2 = 2 * err;
		if (e2 < (i64)(2 * dx + 1) * b2) { ++dx; err += (i64)(2 * dx + 1) * b2; }
		if (e2 > (i64)-(2 * dy - 1) * a2) { --dy; err -= (i64)(2 * dy - 1) * a2; }
	} while (dy >= 0);
	/* fehlerhafter Abbruch bei flachen Ellipsen (b=1) */
	while (dx++ < a) {
		/* Spitze der Ellipse vollenden */
		draw_pixel(x0 + dx, y0, color);
		draw_pixel(x0 - dx, y0, color);
	}
}


// return index to lightmap between 0 and 31 
u8 draw_calculate_shade_from_distance(f32 dis){
	f32 max_render_distance = 100.0f;
	dis = CLAMP(dis, 0.0f, max_render_distance);

	// Map to [0, 31]
	u8 shade = (u8)(dis / max_render_distance * 31.0f);

	return shade;
}

void draw_wall_3d(Player* player, WallRenderingInfo* now, u32 rd) {
	// recursion depth
	if (rd > 32) return;
	Sector sec = *map_get_sector(now->sectorno);
	for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {

		Wall w = *map_get_wall(i);
		//world pos
		v2 p1 = world_pos_to_camera(w.a, player->pos, player->anglesin, player->anglecos);
		v2 p2 = world_pos_to_camera(w.b, player->pos, player->anglesin, player->anglecos);

		v2 tp1 = p1;
		v2 tp2 = p2;

		f32 a1 = atan2f(p1.y, p1.x) - PI_2;
		f32 a2 = atan2f(p2.y, p2.x) - PI_2;

		//calculate intersection between walls and view frustum and clip walls
		if (p1.y <= ZNEAR || p2.y <= ZNEAR || a1 >= +(HFOV / 2) || a2 <= -(HFOV / 2)) {
			v2 il;
			i32 hitl = get_line_intersection(p1, p2, znl, zfl, &il);
			v2 ir;
			i32 hitr = get_line_intersection(p1, p2, znr, zfr, &ir);
			if (hitl && hitr) {
				if (p1.x - p2.x > 0) continue;
			}
			if (hitl) {
				p1 = il;
				a1 = atan2f(p1.y, p1.x) - PI_2;
			}
			if (hitr) {
				p2 = ir;
				a2 = atan2f(p2.y, p2.x) - PI_2;
			}
		}
		if (a1 < a2 || a2 < -(HFOV / 2) - 0.01f || a1 > +(HFOV / 2) + 0.01f) continue;


		//convert the angle of the wall into screen coordinates (player FOV is 90 degrees or 1/2 PI)
		i32 tx1 = screen_angle_to_x(a1);
		i32 tx2 = screen_angle_to_x(a2);
		if (tx1 != 0) tx1++;

		if (tx1 > now->sx2) continue;
		if (tx2 < now->sx1) continue;

		i32 x1 = CLAMP(tx1, now->sx1, now->sx2);
		i32 x2 = CLAMP(tx2, now->sx1, now->sx2);

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
		
		floorplane = draw_find_plane(sec.zfloor, 0);
		ceilplane = draw_find_plane(sec.zceil, 0);

		i32 x2_test = CLAMP(x2 + 1, 0, SCREEN_WIDTH-1);
		i32 x1_test = CLAMP(x1 - 1, 0, SCREEN_WIDTH - 1);
		floorplane = draw_check_plane(floorplane, x1_test, x2_test);
		ceilplane = draw_check_plane(ceilplane, x1_test, x2_test);

		// get floor and ceiling height of sector behind wall if wall is a portal
		f32 nzfloor = sec.zfloor;
		f32 nzceil = sec.zceil;

		if (w.portal >= 0) {
			Sector portal = *map_get_sector(w.portal);
			nzfloor = portal.zfloor;
			nzceil = portal.zceil;
		}

		f32 sy0 = (VFOV * SCREEN_HEIGHT) / p1.y;
		f32 sy1 = (VFOV * SCREEN_HEIGHT) / p2.y;

		// wall coordinates
		i32 yf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player->z) * sy0);
		i32 yf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player->z) * sy1);
		i32 yc0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player->z) * sy0);
		i32 yc1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player->z) * sy1);
		// bottom wall coordinates if bottom of wall would be at original height, used for absolute texture drawing regardless of floor and ceil height
		i32 yaf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor_old - player->z) * sy0);
		i32 yaf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor_old - player->z) * sy1);

		// portal coordinates in the wall
		i32 pf0 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player->z) * sy0);
		i32 pf1 = (SCREEN_HEIGHT / 2) + (i32)((nzfloor - player->z) * sy1);
		i32 pc0 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player->z) * sy0);
		i32 pc1 = (SCREEN_HEIGHT / 2) + (i32)((nzceil - player->z) * sy1);

		// wall texture mapping varaibles
		v2 difp1 = v2_sub(p1, tp1);
		v2 difp2 = v2_sub(p2, tp2);
		f32 twlen = v2_len(v2_sub(tp1, tp2));
		v2 cutoff = { fabsf(v2_len(difp1) / twlen), fabsf(v2_len(difp2) / twlen) };

		for (i32 x = x1; x <= x2; x++) {
			//calculate x stepsize
			f32 xp = (x - tx1) / (f32)(tx2 - tx1);

			//get top and bottom coordinates of the wall
			i32 tyf = (i32)(xp * (yf1 - yf0)) + yf0;
			i32 tyc = (i32)(xp * (yc1 - yc0)) + yc0;
			i32 tayf = (i32)(xp * (yaf1 - yaf0)) + yaf0; // bottom of the wall, if wall had height 0, used for absolute texture drawing of walls
			i32 yf = CLAMP(tyf, floorclip[x], ceilingclip[x]);
			i32 yc = CLAMP(tyc, floorclip[x], ceilingclip[x]);
			
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

			f32 u = ((1.0f - a) * (u0 / z0) + a * (u1 / z1)) / ((1.0f - a) * 1 / z0 + a * (1.0f / z1));

			//wall distance for lightlevel calc
			f32 dis = tp1.y * (1 - u) + tp2.y * (u);

			u8 wallshade_index = draw_calculate_shade_from_distance(dis);

			f32 wallheight;

			f32 dx = w.a.x - w.b.x;
			f32 dy = w.a.y - w.b.y;
			f32 wallwidth = sqrtf(dx * dx + dy * dy);

			//draw Wall
			if (w.portal == -1 && !w.transparent) {
				//drawVerticalLine(x, yf, yc, color, pixels); wall in one color
				if (yc > tyf && yf < tyc) {
					f32 wallheight = sec.zceil - sec.zfloor;
					draw_tex_line(x, yf, yc, tyf, tyc, tayf, u, wallshade_index, dis, sec.zfloor, sec.zfloor_old, wallheight, wallwidth, &w, WALL, false);
				}
				ceilingclip[x] = 0;
				floorclip[x] = SCREEN_HEIGHT - 1;
			}

			//draw Portal
			else {
				//get top and bottom coordinates of the portal
				i32 tpyf = (i32)(xp * (pf1 - pf0)) + pf0;
				i32 pyf = CLAMP(tpyf, yf, yc);
				i32 tpyc = (i32)(xp * (pc1 - pc0)) + pc0;
				i32 pyc = CLAMP(tpyc, yf, yc);

				//if neighborfloor is higher than current sectorceiling then draw it
				//if (pyf > yf) { drawVerticalLine(x, yf, pyf, YELLOW, pixels); }
				if (pyf > yf) { 
					wallheight = nzfloor - sec.zfloor;
					draw_tex_line(x, yf, pyf, tyf, tpyf, tayf, u, wallshade_index, dis, sec.zfloor, sec.zfloor_old, wallheight, wallwidth, &w, PORTAL_LOWER, false);
				}
				//draw window
				//drawVerticalLine(x, pyf, pyc, color, pixels);
				//if neighborceiling is lower than current sectorceiling then draw it
				if (pyc < yc) { 
					wallheight = sec.zceil - nzceil;
					draw_tex_line(x, pyc, yc, tpyc, tyc, tayf, u, wallshade_index, dis, sec.zfloor, sec.zfloor_old, wallheight, wallwidth, &w, PORTAL_UPPER, false);
				}

				//update vertical clipping arrays
				ceilingclip[x] = CLAMP(pyc, 0, ceilingclip[x]);
				floorclip[x] = CLAMP(pyf, floorclip[x], SCREEN_HEIGHT - 1);
			}
		}
		drawn_sectors[now->sectorno] = true;
		if (w.portal >= 0 && !now->renderedSectors[w.portal]) {
			WallRenderingInfo* wr = &(WallRenderingInfo){ w.portal, x1, x2};
			memcpy(wr->renderedSectors, now->renderedSectors, SECTOR_MAX * sizeof(u8));
			wr->renderedSectors[now->sectorno] = true;
			draw_wall_3d(player, wr, ++rd);
		}
	}
}


void draw_transparent_walls(Player* player) {
	for (size_t i = 0; i < map_get_sectoramt(); i++) {
		if (!drawn_sectors[i]) continue;
		Sector sec = *map_get_sector(i);
		for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {
			Wall w = *map_get_wall(i);
			if (!w.transparent) continue;
			for (size_t k = 0; k < 2; k++) {
				// try drawing the backside of the wall
				if (k == 1) {
					v2 b = w.b;
					w.b = w.a;
					w.a = b;
				}

				//world pos
				v2 p1 = world_pos_to_camera(w.a, player->pos, player->anglesin, player->anglecos);
				v2 p2 = world_pos_to_camera(w.b, player->pos, player->anglesin, player->anglecos);

				v2 tp1 = p1;
				v2 tp2 = p2;

				f32 a1 = atan2f(p1.y, p1.x) - PI_2;
				f32 a2 = atan2f(p2.y, p2.x) - PI_2;

				//calculate intersection between walls and view frustum and clip walls
				if (p1.y <= ZNEAR || p2.y <= ZNEAR || a1 >= +(HFOV / 2) || a2 <= -(HFOV / 2)) {
					v2 il;
					i32 hitl = get_line_intersection(p1, p2, znl, zfl, &il);
					v2 ir;
					i32 hitr = get_line_intersection(p1, p2, znr, zfr, &ir);
					if (hitl && hitr) {
						if (p1.x - p2.x > 0) continue;
					}
					if (hitl) {
						p1 = il;
						a1 = atan2f(p1.y, p1.x) - PI_2;
					}
					if (hitr) {
						p2 = ir;
						a2 = atan2f(p2.y, p2.x) - PI_2;
					}
				}
				if (a1 < a2 || a2 < -(HFOV / 2) - 0.01f || a1 > +(HFOV / 2) + 0.01f) continue;

				//convert the angle of the wall into screen coordinates (player FOV is 90 degrees or 1/2 PI)
				i32 x1 = screen_angle_to_x(a1);
				i32 x2 = screen_angle_to_x(a2);

				f32 sy0 = (VFOV * SCREEN_HEIGHT) / p1.y;
				f32 sy1 = (VFOV * SCREEN_HEIGHT) / p2.y;

				// wall coordinates
				i32 yf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player->z) * sy0);
				i32 yf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor - player->z) * sy1);
				i32 yc0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player->z) * sy0);
				i32 yc1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zceil - player->z) * sy1);
				// bottom wall coordinates if bottom of wall would be at original heigh, used for absolute texture drawing regardless of floor and ceil height
				i32 yaf0 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor_old - player->z) * sy0);
				i32 yaf1 = (SCREEN_HEIGHT / 2) + (i32)((sec.zfloor_old - player->z) * sy1);


				// wall texture mapping varaibles
				v2 difp1 = v2_sub(p1, tp1);
				v2 difp2 = v2_sub(p2, tp2);
				f32 twlen = v2_len(v2_sub(tp1, tp2));
				v2 cutoff = { fabsf(v2_len(difp1) / twlen), fabsf(v2_len(difp2) / twlen) };

				for (i32 x = x1; x <= x2; x++) {
					//calculate x stepsize
					f32 xp = (x - x1) / (f32)(x2 - x1);

					//get top and bottom coordinates of the wall
					i32 tyf = (i32)(xp * (yf1 - yf0)) + yf0;
					i32 tyc = (i32)(xp * (yc1 - yc0)) + yc0;
					i32 tayf = (i32)(xp * (yaf1 - yaf0)) + yaf0; // bottom of the wall, if wall had height 0, used for absolute texture drawing of walls
					i32 yf = CLAMP(tyf, 0, SCREEN_HEIGHT - 1);
					i32 yc = CLAMP(tyc, 0, SCREEN_HEIGHT - 1);

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

					f32 u = ((1.0f - a) * (u0 / z0) + a * (u1 / z1)) / ((1.0f - a) * 1 / z0 + a * (1.0f / z1));

					//wall distance for lightlevel calc
					f32 dis = tp1.y * (1 - u) + tp2.y * (u);

					u8 wallshade_index = draw_calculate_shade_from_distance(dis);

					f32 wallheight;

					f32 dx = w.a.x - w.b.x;
					f32 dy = w.a.y - w.b.y;
					f32 wallwidth = sqrtf(dx * dx + dy * dy);

					//draw transparent Wall
					if (yc > tyf && yf < tyc) {
						f32 wallheight = sec.zceil - sec.zfloor;
						bool mirrored = k == 1 ? true : false;
						draw_tex_line(x, yf, yc, tyf, tyc, tayf, u, wallshade_index, dis, sec.zfloor, sec.zfloor_old, wallheight, wallwidth, &w, WALL, mirrored);
					}
				}
			}
		}
	}
}



// x: screen pixel position of the current wallstrip
// y0: clipped screen pixel position of bottom part of wall with lower screen edge
// y1: clipped screen pixel position of top part of wall with upper screen edge
// yf: true screen pixel position of bottom part of wall
// yc: true screen pixel position of top part of wall
// ayf: absolute pixel position of bottom of the wall, if bottom of the wall would be on height 0, used for absolute texture mapping for non portal walls
// u: horizontal position on the texture
// tex: the texture the wall should have
// shade: how dark the wall should be, index to lightmap
// dis: distane wall wallstrip to the camera
// sec_floor_height: floor height of the sector
// wallwidth: how long is the wall
// wall: pointer to the wall
// is_upper_portals: bool that describes if the wallsegement is an upper part of a portal
// wall_backside: if backside of the wall gets drawn, so walltexture gets flipped and decal get drawm with respect if they are on the front or back
void draw_tex_line(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, i32 ayf, f32 u, u8 shade_index, f32 dis, f32 zfloor, f32 zfloor_old, f32 wallheight, f32 wallwidth, Wall* wall, wall_section_type type, bool wall_backside) {
	// draw decals
	if (wall_backside) u = 1.0f - u;
	f32 wall_pos_x = u * wallwidth;
	
	bool wall_drawn[SCREEN_HEIGHT] = { false };
	// draw decals on the front
	if (!wall->transparent) {
		for (Decal* d = wall->decalhead; d != NULL; d = d->next) {
			if (d->wall_type != type || d->front == wall_backside) continue;
			f32 rel_decal_height = map_decal_wall_height(d, wall, zfloor);
			// if the decal is on the top portal, then sbtract the height of the neighbouring sector, as the thats where the bottom of the wall
			if (d->wall_type == PORTAL_UPPER) {
				Sector* sec = map_get_sector(wall->portal);
				rel_decal_height -= sec->zceil;
			}
			// if decal is above wall
			if (rel_decal_height > wallheight || rel_decal_height < -d->size.y) continue;
			// decal not in current stripe of wall, check next decal
			if (!(d->wallpos.x < wall_pos_x && (d->wallpos.x + d->size.x) > wall_pos_x)) continue;
			LightmapindexTexture* decal_tex_ind = &index_textures[d->tex_num];

			f32 pos_x = wall_pos_x - d->wallpos.x;
			f32 tx = (pos_x / d->size.x) * decal_tex_ind->width;

			f32 top_y = (rel_decal_height + d->size.y) / wallheight;
			f32 bot_y = rel_decal_height / wallheight;
			i32 top_ty = (i32)(top_y * (yc - yf) + yf);
			i32 bot_ty = (i32)(bot_y * (yc - yf) + yf);

			i32 bot_ty_clamp = CLAMP(bot_ty, y0, y1);
			i32 top_ty_clamp = CLAMP(top_ty, y0, y1);

			v3 tex_res = calc_tex_start_and_step(bot_ty, top_ty, CLAMP(bot_ty, y0, y1), CLAMP(top_ty, y0, y1), decal_tex_ind->height, 1.0f);
			f32 ty = tex_res.x;
			f32 ty_step = tex_res.z;

			for (i32 y = bot_ty_clamp; y < top_ty_clamp; y++) {
				u8 index = decal_tex_ind->indices[((i32)ty) * decal_tex_ind->width + (i32)tx];
				if (index) {
					draw_pixel_from_lightmap(x, y, index, shade_index);
					wall_drawn[y] = true;
					zBuffer[y * SCREEN_WIDTH + x] = dis;
				}
				ty += ty_step;
			}
		}
	}

	// draw walls
	u32 wall_tex_num = wall->tex;
	LightmapindexTexture* wall_texture_ind = &index_textures[wall_tex_num];

	f32 texture_scale = 10.0f;
	f32 texheight = wallheight / texture_scale;
	f32 texwidth = wallwidth / texture_scale;

	i32 tx = (i32)((u * texwidth) * (wall_texture_ind->width - 1)) % wall_texture_ind->width;

	f32 ty;
	f32 ty_step;

	if (wall->portal >= 0) {
		v3 tex_res = calc_tex_low_high_and_step(yf, yc, y0, y1, wall_texture_ind->height, texheight);
		// lower parts of portals should get drawn from the top
		ty = tex_res.y;
		ty_step = tex_res.z;
		// upper parts of portals should get drawn from the bottom
		if (type == PORTAL_UPPER) ty = tex_res.x;
	}
	else {
		f32 abs_wallheight = (wallheight + (zfloor - zfloor_old));
		v2 tex_res = calc_tex_start_and_step_abs(ayf, yf, yc, y0, y1, wall_texture_ind->height, abs_wallheight / texture_scale);
		ty = tex_res.x;
		ty_step = tex_res.y;
	}

	// don't check for transparancy and zbuffer if wall is a normal wall
	if (!wall->transparent) {
		for (i32 y = y0; y <= y1; y++) {
			// only draw wall when there is no decal
			if (!wall_drawn[y]) {
				u8 index = wall_texture_ind->indices[((i32)(ty-1) % wall_texture_ind->height) * wall_texture_ind->width + (i32)tx];
				draw_pixel_from_lightmap(x, y, index, shade_index);
				wall_drawn[y] = true;
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}
			ty += ty_step;
		}
	}
	else {
		for (i32 y = y0; y <= y1; y++) {
			// only draw wall when there is no decal
			if (!wall_drawn[y] && zBuffer[y * SCREEN_WIDTH + x] > dis) {
				u8 index = wall_texture_ind->indices[((i32)(ty) % wall_texture_ind->height) * wall_texture_ind->width + (i32)tx];
				if (index && zBuffer[y * SCREEN_WIDTH + x] > dis) {
					draw_pixel_from_lightmap(x, y, index, shade_index);
					wall_drawn[y] = true;
					zBuffer[y * SCREEN_WIDTH + x] = dis;
				}
			}
			ty += ty_step;
		}
	}


	// draw on front of transparent walls 
	if (wall->transparent) {
		for (Decal* d = wall->decalhead; d != NULL; d = d->next) {
			if (d->wall_type != type || d->front == wall_backside) continue;
			f32 rel_decal_height = map_decal_wall_height(d, wall, zfloor);
			// if the decal is on the top portal, then sbtract the height of the neighbouring sector, as the thats where the bottom of the wall
			if (d->wall_type == PORTAL_UPPER) {
				Sector* sec = map_get_sector(wall->portal);
				rel_decal_height -= sec->zceil;
			}
			// if decal is above wall
			if (rel_decal_height > wallheight || rel_decal_height < -d->size.y) continue;
			// decal not in current stripe of wall, check next decal
			if (!(d->wallpos.x < wall_pos_x && (d->wallpos.x + d->size.x) > wall_pos_x)) continue;
			LightmapindexTexture* decal_tex_ind = &index_textures[d->tex_num];

			f32 pos_x = wall_pos_x - d->wallpos.x;
			f32 tx = (pos_x / d->size.x) * decal_tex_ind->width;

			f32 top_y = (rel_decal_height + d->size.y) / wallheight;
			f32 bot_y = rel_decal_height / wallheight;
			i32 top_ty = (i32)(top_y * (yc - yf) + yf);
			i32 bot_ty = (i32)(bot_y * (yc - yf) + yf);

			i32 bot_ty_clamp = CLAMP(bot_ty, y0, y1);
			i32 top_ty_clamp = CLAMP(top_ty, y0, y1);

			v3 tex_res = calc_tex_start_and_step(bot_ty, top_ty, CLAMP(bot_ty, y0, y1), CLAMP(top_ty, y0, y1), decal_tex_ind->height, 1.0f);
			f32 ty = tex_res.x;
			f32 ty_step = tex_res.z;

			for (i32 y = bot_ty_clamp; y < top_ty_clamp; y++) {
				// only draw pixel of decal if pixel has not already been drawn by other decal
				if (wall_drawn[y]) {
					u8 index = decal_tex_ind->indices[((i32)ty) * decal_tex_ind->width + (i32)tx];
					if (index) {
						draw_pixel_from_lightmap(x, y, index, shade_index);
						wall_drawn[y] = true;
						zBuffer[y * SCREEN_WIDTH + x] = dis;
					}
				}
				ty += ty_step;
			}
		}
	}
}

void draw_clear_planes() {
	//clear zBuffer
	for (i32 i = 0; i < SCREEN_HEIGHT * SCREEN_WIDTH; i++) { zBuffer[i] = ZFAR; }

	//draw Walls and floor
	for (int i = 0; i < SCREEN_WIDTH; i++) {
		ceilingclip[i] = SCREEN_HEIGHT - 1;
		floorclip[i] = 0;
	}
	lastvisplane = visplanes;
}

visplane_t* draw_find_plane(f32 height, i32 picnum) {
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

visplane_t* draw_check_plane(visplane_t* v, i32 start, i32 stop) {
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

	if (x > intrh) {
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

	for (i32 i = 0; i < SCREEN_WIDTH; i++) {
		v->bottom[i] = SCREEN_HEIGHT + 1;
		v->top[i] = 0;
	}

	return v;
}

void draw_make_spans(i32 x, i32 t1, i32 b1, i32 t2, i32 b2, visplane_t* v, Player* player) {
	while (t1 > t2 && t1 >= b1) {
		draw_map_plane(t1, spanstart[t1], x, v, player);
		t1--;
	}
	while (b1 < b2 && b1 <= t1) {
		draw_map_plane(b1, spanstart[b1], x, v, player);
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

void draw_map_plane(i32 y, i32 x1, i32 x2, visplane_t* v, Player* player) {
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
	f32 dis = (f32)fabs(((player->z - v->height) * yslope[y]));
	//relative coordinates to player
	f32 xt = -sinf(a) * dis / cosf(a);
	f32 yt =  cosf(a) * dis / cosf(a);
	// absolute coordinates
	v2 p = camera_pos_to_world((v2) { xt, yt }, player->pos, player->anglesin, player->anglecos);

	p.x *= texsizefactor.x;
	p.y *= texsizefactor.y;

	f32 step = dis / (SCREEN_WIDTH / 2);

	f32 xstep = step * cosf(player->angle - PI_2) * texsizefactor.x;
	f32 ystep = step * sinf(player->angle - PI_2) * texsizefactor.y;

	u8 floor_row_shade_index = draw_calculate_shade_from_distance(dis);

	for (i32 x = x1; x <= x2; x++)
	{
		u32 texture_num = 3;
		LightmapindexTexture* decal_tex_ind = &index_textures[texture_num];
		v2i t = { (i32)(p.x) & (texwidth - 1), (i32)(p.y) & (texwidth - 1) };
		u8 index = decal_tex_ind->indices[(texheight - 1 - t.y) * decal_tex_ind->width + t.x];
		draw_pixel_from_lightmap(x, y, index, floor_row_shade_index);
		zBuffer[y * SCREEN_WIDTH + x] = dis;
		p.x += xstep;
		p.y += ystep;
	}
}

void draw_planes_3d(Player* player) {
	
	for (visplane_t* v = visplanes; v < lastvisplane; v++) {
		if (v->minx > v->maxx) continue;
		
		v->top[v->maxx] = 0;
		v->top[v->minx] = 0;

		for (i32 x = v->minx ; x < v->maxx; x++) {
			draw_make_spans(x, v->top[x], v->bottom[x], v->top[x+1], v->bottom[x+1], v, player);
		}
	}

	
	u32 colors[8] = {BLUE,RED,GREEN,YELLOW,PURPLE,ORANGE,WHITE,LIGHTGRAY};
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
				f32 dis = fabs(((player->z - v->height) * yslope[y]));
				//relative coordinates to player
				f32 xt = -sin(a) * dis / (cos(a));
				f32 yt = cos(a) * dis / (cos(a));
				// absolute coordinates
				v2 p = camera_pos_to_world((v2) { xt, yt }, player->pos, player->anglesin, player->anglecos);
				// texutre coordinates
				v2i t = { (i32)((p.x - ((i32)p.x)) * texheight) & (texwidth - 1), (i32)((p.y - ((i32)p.y)) * texheight) & (texwidth - 1)};
				//u32 color = tex[0].pixels[(256 - t.y) * tex[0].width + t.x];
				draw_pixel(x, y, colors[color%8], pixels);
				//drawPixel(x, y, color, pixels);
				zBuffer[y * SCREEN_WIDTH + x] = dis;
			}
		}
	}
}


void draw_sprites(Player* player, EntityHandler* handler) {

	for (u32 i = 0; i < handler->used; i++)
	{
		const Entity e = *handler->entities[i];
		if (!drawn_sectors[e.sector]) continue;
		if (e.type == Projectile){
			f32 dx = e.pos.x - player->pos.x;
			f32 dy = e.pos.y - player->pos.y;
			f32 dis = sqrtf(dx * dx + dy * dy);
			if (dis < 4.0f) {
				continue;
			}
		}

		if (e.relCamPos.y <= 0) continue;

		f32 spritevMove = -SCREEN_HEIGHT  * (e.z - player->z);
		f32 spritea = atan2f(e.relCamPos.y, e.relCamPos.x) - PI / 2;
		i32 spriteScreenX = screen_angle_to_x(spritea);
		i32 vMoveScreen = (i32)(spritevMove / e.relCamPos.y);

		u32 texture_num = 1;
		LightmapindexTexture* sprite_ind = &index_textures[texture_num];

		i32 spriteHeight = (i32)((SCREEN_HEIGHT / e.relCamPos.y) * e.scale.y);
		i32 y0 = -spriteHeight / 2 + SCREEN_HEIGHT / 2 - vMoveScreen;
		i32 y0_clamp = CLAMP(y0, 0, SCREEN_HEIGHT);
		i32 y1 = spriteHeight / 2 + SCREEN_HEIGHT / 2 - vMoveScreen;
		i32 y1_clamp = CLAMP(y1, 0, SCREEN_HEIGHT);

		v3 tex_res_y =  calc_tex_start_and_step(y0, y1, y0_clamp, y1_clamp, sprite_ind->height, 1.0f);
		f32 ty = tex_res_y.x;
		f32 stepy = tex_res_y.z;

		// multiply by 9.0f/16.0f as this is the usual resolution for this game
		i32 spriteWidth = (i32)((SCREEN_WIDTH / e.relCamPos.y) * e.scale.x * (9.0f / 16.0f));
		i32 x0 = -spriteWidth / 2 + spriteScreenX;
		i32 x0_clamp = CLAMP(x0, 0, SCREEN_WIDTH);
		i32 x1 = spriteWidth / 2 + spriteScreenX;
		i32 x1_clamp = CLAMP(x1, 0, SCREEN_WIDTH);

		v3 tex_res_x = calc_tex_start_and_step(x0, x1, x0_clamp, x1_clamp, sprite_ind->width, 1.0f);
		f32 tx_start = 256 - tex_res_x.x;
		f32 stepx = -tex_res_x.z;

		f32 tx;
		u8 shade_index = draw_calculate_shade_from_distance(e.relCamPos.y);
		for (i32 y = y0_clamp; y < y1_clamp; y++) {
			ty += stepy;
			tx = tx_start;
			for (i32 x = x0_clamp; x < x1_clamp; x++) {
				if (zBuffer[y * SCREEN_WIDTH + x] > e.relCamPos.y) {
					u8 index = sprite_ind->indices[((i32)ty % sprite_ind->height) * sprite_ind->width + ((i32)tx % sprite_ind->width)];
					if (index) {
						draw_pixel_from_lightmap(x, y, index, shade_index);
						zBuffer[y * SCREEN_WIDTH + x] = e.relCamPos.y;
					}
				}
				tx += stepx;
			}
		}
	}
}

void draw_minimap(Player* player) {
	v2i mapoffset = (v2i){ SCREEN_WIDTH/20 , SCREEN_HEIGHT - SCREEN_HEIGHT/4 };
	f32 x_scale = SCREEN_WIDTH / 1280.0f;
	f32 y_scale = SCREEN_HEIGHT / 720.0f;
	f32 scale = (x_scale + y_scale) / 2;

	i32 sectornum = map_get_sectoramt();
	for (i32 j = 0; j < sectornum; j++)
	{
		Sector sec = *map_get_sector(j);
		for (i32 i = sec.index; i < (sec.index + sec.numWalls); i++) {
			Wall w = *map_get_wall(i);
			draw_line((i32)(w.a.x * scale + mapoffset.x), (i32)(w.a.y * scale + mapoffset.y), (i32)(w.b.x * scale + mapoffset.x), (i32)(w.b.y * scale + mapoffset.y), WHITE);
		}
	}
	draw_circle((i32)(player->pos.x * scale + mapoffset.x), (i32)(player->pos.y * scale + mapoffset.y), (i32)(3 * scale), (i32)(3 * scale), RED);
	draw_line((i32)(player->pos.x * scale + mapoffset.x), (i32)(player->pos.y * scale + mapoffset.y),
			  (i32)((player->anglecos * 10 + player->pos.x) * scale + mapoffset.x), (i32)((player->anglesin * 10 + player->pos.y) * scale + mapoffset.y), RED);
}


// true_low and true_high are the pixel coordinates of where the sprite is in full
// low and high are the pixel coordinates where the sprite is on screen with clipping
// tex_size is the size of the texture
// scale is how often the texture should be applied to the range from true_low to true_high

// return texture y pos to start and stop and step for each update, textures are anchored at the bottom, so texture drawing starts at y pos 0 at the bottom of the object
v3 calc_tex_start_and_step(i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale) {
	f32 t0 = (1.0f - ((low  - true_low) / (f32)(true_high - true_low))) * (tex_size-1) * scale;
	f32 t1 = (1.0f - ((high - true_low) / (f32)(true_high - true_low))) * (tex_size-1) * scale;
	f32 step = (t1 - t0) / (high - low);
	return(v3) {t0, t1, step};
}

// return texture y pos if texture is anchored at the bottom and texture y pos if texture is anchored at the top, with a step
v3 calc_tex_low_high_and_step(i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale) {
	f32 step = -((tex_size) / (f32)(true_high - true_low));
	// add how many pixels are cut off the bottom
	f32 start_low = (low - true_low) * step - 1;
	// add how many pixels are cut off the top + the pixels in the drawing area, as we always start drawing from the bottom
	f32 start_high = (true_high - low) * -step - 1;
	return (v3) { start_low * scale, start_high * scale, step * scale};
}

// return texture y pos to start and step for each update, textures are anchored at the bottom, so texture drawing starts at y pos 0 at the bottom of the object
v2 calc_tex_start_and_step_abs(i32 true_abs_low, i32 true_low, i32 true_high, i32 low, i32 high, i32 tex_size, f32 scale) {
	f32 step = -((tex_size) / (f32)(true_high - true_abs_low));
	// add how many pixels are cut off the bottom
	f32 start_low = (low - true_abs_low) * step - 1;
	return (v2) { start_low * scale, step * scale };
}

void draw_2d() {
	// draw crosshair
	draw_fill_rectangle(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT / 2 + 1, GREEN);
	draw_fill_rectangle(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2 - 8, SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2 + 8, GREEN);
}