#pragma once
#include "util.h"
#include "ticker.h"
#include "tick.h"

typedef enum EntityType {
	Enemy,
	Item
}Entitytype;

typedef struct {
	ticker tick;
	Entitytype type;
	v2 pos;
	f32 angle;
	v2 relCamPos;
	u32 spriteNum[8];
	u8 spriteAmt;
	v2 scale;
	f32 vMove;
	i32 health;
	i32 damage;
	f32 speed;
	i32 animationtick;
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