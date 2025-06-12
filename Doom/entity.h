#pragma once
#include "util.h"
#include "ticker.h"
#include "tick.h"
#include "player.h"

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
	v3 velocity;
	u32 spriteNum[8];
	u8 spriteAmt;
	v2 scale;
	f32 health, damage, speed;
	f32 animationtick;
	u8 inAir;
	u32 sector;
	Player* target;
	bool dirty;
} Entity;

typedef struct EntityHandler_s EntityHandler;

void calc_all_rel_cam_pos(EntityHandler* handler, Player* player);
void tick_item(Entity* item);
void tick_enemy(Entity* enemy);
void tick_bullet(Entity* bullet);
void check_entity_collisions(EntityHandler* handler, Player* player);
