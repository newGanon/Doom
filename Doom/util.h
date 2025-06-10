#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

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
typedef struct v2u8_s { u8 x, y; } v2u8;

typedef struct v3_s { f32 x, y, z; } v3;
typedef struct v3i_s { i32 x, y, z; } v3i;
typedef struct v3u8_s { u8 x, y, z; } v3u8;

#define PI   3.14159265359f
#define PI_2 1.57079632679f
#define PI_4 0.78539816339f

#define SCREEN_WIDTH 1280	
#define SCREEN_HEIGHT 720
//#define SCREEN_WIDTH 1920	
//#define SCREEN_HEIGHT 1080

#define EYEHEIGHT 6.0f
#define SNEAKHEIGHT 4.5f
#define HEADMARGIN 1.0f 
#define STEPHEIGHT 2.0f
#define PLAYERSPEED 20.0f
#define PLAYERSNEAKSPEED 10.0f

#define HFOV PI_2
#define VFOV 1.0f   
#define LIGHTDIMINISHINGDFACTOR 0.1f
#define PLAYERTOATIONSPEED 0.001f

//#define ZNEAR 0.0001f
#define ZNEAR 0.0f
#define ZFAR 32768.0f

#define SECTOR_MAX 256
#define WALL_MAX 2048
#define MAXVISPLANES 4096

#define MAXPLATFORMS 100

#define SCREEN_FPS 240
//#define SECONDS_PER_FRAME (1.0f / SCREEN_FPS)
//#define MS_PER_UPDATE (1000.0f / SCREEN_FPS)
#define MS_PER_UPDATE 10.0f
#define SECONDS_PER_UPDATE (MS_PER_UPDATE / 1000)

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

#define GRAVITY 80.0f

#define A(c) (((c) >> 24) & 0xFF)
#define R(c) (((c) >> 16) & 0xFF)
#define G(c) (((c) >> 8) & 0xFF)
#define B(c) ((c) & 0xFF)
