#pragma once
#include "util.h"

typedef struct EntityHandler_s EntityHandler;

typedef struct Player_s {
	v2 pos;
	f32 z;
	v3 velocity;
	f32 angle, anglecos, anglesin;
	f32 speed;
	u32 sector;
	bool shoot, airborne, dead, sneak;
}Player;

void player_tick(Player* p, EntityHandler* handler, bool* KEYS);
void player_check_shoot(Player* p, EntityHandler* handler);
void player_trymove(Player* p);
