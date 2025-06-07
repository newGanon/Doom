#pragma once
#include "util.h"

typedef struct Player {
	v2 pos;
	f32 z;
	v3 velocity;
	f32 angle, anglecos, anglesin;
	f32 speed;
	u32 sector;
	bool shoot, in_air, dead, sneak;
}Player;

void player_tick(Player* p, bool* KEYS);
void check_shoot(Player* p);
