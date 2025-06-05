#pragma once
#include "util.h"
#include "ticker.h"
#include "map.h"

typedef enum {
	ELEVATOR
} PLAT_TYPE;

typedef struct Platform {
	ticker tick;
	Sector sec;
	f32 speed;
	f32 low;
	f32 high;
	PLAT_TYPE type;

} Platform;