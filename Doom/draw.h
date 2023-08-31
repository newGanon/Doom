#pragma once
#include "util.h"
#include "sdl.h"

typedef struct {
	u32* pixels;
	u32 width;
	u32 height;
}Texture;  

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

void drawPixel(i32 x, i32 y, u32 color, u32* pixels);
void drawInit();
void drawVerticalLine(i32 x, i32 y0, i32 y1, u32 color, u32* pixels);
void drawLine(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels);
void drawSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels);
void fillSquare(i32 x0, i32 y0, u32 size, u32 color, u32* pixels);
void fillRectangle(i32 x0, i32 y0, i32 x1, i32 y1, u32 color, u32* pixels);
void drawCircle(i32 x0, i32 y0, i32 a, i32 b, u32 color, u32* pixels);
void draw3D(Player player, Map* map, u32* pixels, Texture* tex);
void drawTexLine(i32 x, i32 y0, i32 y1, i32 yf, i32 yc, f64 u, Texture* tex, f32 shade, f32 dis, u32* pixels);
void drawWall3D(Player player, Map* map, u32* pixels, Texture* tex, WallRenderingInfo* now, u32 rd);
void drawPlanes3D(Player player, Map* map, u32* pixels, Texture* tex);
void makeSpans(i32 x, i32 t1, i32 b1, i32 t2, i32 b2, visplane_t* v, Player player, Map* map, u32* pixels, Texture* tex);
void mapPlane(i32 y, i32 x1, i32 x2, visplane_t* v, Player player, Map* map, u32* pixels, Texture* tex);
void drawSprites(Player player, Map* map, u32* pixels, Texture* tex);
void drawMinimap(Player player, Map* map, u32* pixels);
void clearPlanes();
visplane_t* findPlane(f32 height, i32 picnum);
visplane_t* checkPlane(visplane_t* v, i32 start, i32 stop);

u32 changeRGBBrightness(u32 color, f32 factor);
f32 calcWallShade(v2 start, v2 end, f32 dis);
f32 calcFlatShade(f32 dis);