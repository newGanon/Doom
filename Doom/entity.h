#pragma once
#include "util.h"
#include "ticker.h"
#include "tick.h"
#include "player.h"

typedef enum EntityType {
	Enemy,
	Item,
	Projectile
}Entitytype;

typedef struct {
	ticker tick;
	Entitytype type;
	v2 pos, relCamPos;
	f32 z;
	v3 velocity, acceleration;
	u32 spriteNum[8];
	u8 spriteAmt;
	v2 scale;
	f32 vMove;
	i32 health, damage, speed;
	i32 animationtick;
	u8 inAir;
	Player* target;
}Entity;

typedef struct {
	Entity* entities;
	u32 used;
	u32 size;
}EntityHandler;

void addEntity(EntityHandler* h, Entity entity);
void initEntityHandler(EntityHandler* h, u32 initialSize);
void freeEntities(EntityHandler* h);
void removeEntityAt(EntityHandler* h, i32 index);
void removeEntity(EntityHandler* h, Entity* e);
void calcAllRelCamPos(EntityHandler* h, Player* player);
void sortEntities(EntityHandler* handler, Player* player);
void tick_item(Entity* item);
void tick_enemy(Entity* enemy);