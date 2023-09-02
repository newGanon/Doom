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
	v3 velocity;
	u32 spriteNum[8];
	u8 spriteAmt;
	v2 scale;
	i32 health, damage, speed;
	i32 animationtick;
	u8 inAir;
	u32 sector;
	Player* target;
}Entity;

typedef struct {
	Entity** entities;
	u32 used;
	u32 size;
}EntityHandler;

void addEntity(Entity* e);
void initEntityHandler(EntityHandler* h, u32 initialSize);
void free_entitiehandler();
void removeEntity(Entity* e);
void calcAllRelCamPos(Player* player);
void sortEntities(Player* player);
void tick_item(Entity* item);
void tick_enemy(Entity* enemy);
void tick_bullet(Entity* bullet);