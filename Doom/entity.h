#pragma once
#include "util.h"
#include "tick.h"

typedef struct EntityHandler_s EntityHandler;
typedef struct Player_s Player;

typedef enum EntityType_e {
	Enemy,
	Item,
	Projectile
}Entitytype;

typedef struct Entity_s{
	ticker tick;
	Entitytype type;
	v2 pos, relCamPos;
	f32 z;
	v2 dir;
	v3 velocity;
	u32 spriteNum[8];
	u8 spriteAmt;
	v2 scale;
	f32 health, damage, speed;
	f32 animationtick;
	u32 sector;
	u8 airborne;
	Player* target;
	bool dirty;
} Entity;


void tick_item(Entity* item);
void tick_enemy(Entity* enemy);
void tick_bullet(Entity* bullet);
void entity_check_collisions(EntityHandler* handler, Player* player);
void entity_calculate_relative_camera_position(EntityHandler* handler, Player* player);
bool entity_trymove(Entity* e, bool gravityactive);