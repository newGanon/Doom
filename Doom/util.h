#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a < b ? b : a)
#define VETORCROSSPROD2D(x0, x1, y0, y2) ((x0 * y1) - (y0 * x1))
#define OVERLAP2D(a0, a1, b0, b1) (MIN(a0, a1) <= MAX(b0, b1) && MIN(b0, b1) <= MAX(a0, b0))
#define BOXINTERSECT2D(x0, y0, x1, y1, x2, y2, x3, y3) ((OVERLAP2D(x0, x1, x2, x3) && OVERLAP2D(y0, y1, y2, y3))
#define POINTSIDE2D(px, py, x0, y0, x1, y1) ((x1 - x0) * (py - y0) - (y1 - y0) * (px - x0)) 

typedef float    f32;
typedef double   f64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef size_t   usize;

static unsigned NumSectors = 0;

typedef struct v2_s { f32 x, y; } v2;
typedef struct v2i_s { i32 x, y; } v2i;
typedef struct v2u_s8 { i32 x, y; } v2u8;

typedef struct v3_s { f32 x, y, z; } v3;
typedef struct v3i_s { i32 x, y, z; } v3i;
typedef struct v3u_s8 { i32 x, y, z; } v3u8;

#define PI 3.14159265359f
#define PI_2 (PI / 2.0f)
#define PI_4 (PI / 4.0f)

#define SCREEN_WIDTH 1280	
#define SCREEN_HEIGHT 720
#define EYEHEIGHT 6
#define SNEAKHEIGHT 2.5  
#define HEADMARGIN 1   
#define STEPHEIGHT 2    
#define HFOV PI_2
#define VFOV 0.5f   

#define ZNEAR 0.0001f
#define ZFAR 32768.0f

#define SECTOR_MAX 256

#define SCREEN_FPS 500
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define YELLOW 0xFFe9fa34
#define PURPLE 0xFFcd22f0
#define BLACK 0xFF000000
#define ORANGE 0xFFd19617
#define LIGHTGRAY 0xFF757575
#define DARKGRAY 0xFF424242
#define WHITE 0xFFFFFFFF

typedef struct {
	v3 pos, velocity;
	f32 angle, anglecos, anglesin;
	u32 sector;
}Player;

typedef struct Sector {
	i32 id, index, numWalls;
	f32 zfloor, zceil;
}Sector;

typedef struct Wall {
	v2 a, b;
	u8 portal;
}Wall;

typedef struct Map {
	Wall walls[256];
	u8 wallnum;
	Sector sectors[64];
	u8 sectornum;
} Map;