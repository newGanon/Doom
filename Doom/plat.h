#pragma once
#include "util.h"
#include "ticker.h"
#include "map.h"

typedef enum {
	INFINITE_UP_DOWN
} plat_type;

typedef enum {
	UP,
	DOWN,
	WAIT
} plat_status;

typedef struct Platform {
	ticker tick;
	Sector* sec;
	f32 speed;
	f32 low;
	f32 high;
	plat_status status;
	plat_type type;
	bool floor;
	bool reverseable;
} Platform;

void create_plat(i32 tag, plat_type type, bool floor);