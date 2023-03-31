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

#define SCREEN_WIDTH 1280	
#define SCREEN_HEIGHT 720

#define SCREEN_FPS 500
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define PI 3.14159265359f

#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define YELLOW 0xFFe9fa34
#define PURPLE 0xFFcd22f0
#define BLACK 0xFF000000
#define ORANGE 0xd19617
#define LIGHTGRAY 0x757575
#define DARKGRAY 0x424242
#define WHITE 0xFFFFFFFF