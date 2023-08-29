#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

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

typedef struct v2_s { f32 x, y; } v2;
typedef struct v2i_s { i32 x, y; } v2i;
typedef struct v2u_s8 { i32 x, y; } v2u8;

typedef struct v3_s { f32 x, y, z; } v3;
typedef struct v3i_s { i32 x, y, z; } v3i;
typedef struct v3u_s8 { i32 x, y, z; } v3u8;

#define PI   3.14159265359f
#define PI_2 1.57079632679f
#define PI_4 0.78539816339f

#define SCREEN_WIDTH 1280	
#define SCREEN_HEIGHT 720
#define EYEHEIGHT 6
#define SNEAKHEIGHT 2.5f
#define HEADMARGIN 1   
#define STEPHEIGHT 2    
#define HFOV PI_2
#define VFOV 0.5f   
#define LIGHTDIMINISHINGDFACTOR 0.1f
#define PLAYERTOATIONSPEED 0.001f

#define ZNEAR 0.0001f
#define ZFAR 32768.0f

#define SECTOR_MAX 256
#define WALL_MAX 2048
#define MAXVISPLANES 4096

#define SCREEN_FPS 240
#define SCREEN_TICKS_PER_FRAME (1000 / SCREEN_FPS)

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

#define LIGHTING 0

typedef struct {
	v3 pos, velocity;
	f32 angle, anglecos, anglesin;
	u32 sector;
	u8 inAir;
}Player;

typedef struct Sector {
	i32 id, index, numWalls;
	f32 zfloor, zceil;
}Sector;

typedef struct Wall {
	v2 a, b;
	i32 portal;
	f32 distance;
}Wall;

typedef struct Map {
	Wall walls[WALL_MAX];
	i32 wallnum;
	Sector sectors[SECTOR_MAX];
	i32 sectornum;
} Map;

typedef struct visplane_t
{
	f32	height;
	i32	picnum;
	i32	lightlevel;
	i32	minx;
	i32	maxx;
	u16 top[SCREEN_WIDTH];
	u16 bottom[SCREEN_WIDTH];

} visplane_t;

f32 yslope[SCREEN_HEIGHT];
f32 screenxtoangle[SCREEN_WIDTH];

f32 zBuffer[SCREEN_HEIGHT * SCREEN_WIDTH];

u16 ceilingclip[SCREEN_WIDTH * SECTOR_MAX];
u16 floorclip[SCREEN_WIDTH * SECTOR_MAX];

i32 spanstart[SCREEN_HEIGHT];