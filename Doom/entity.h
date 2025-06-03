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
	f32 animationtick;
	u8 inAir;
	u32 sector;
	Player* target;
}Entity;

typedef struct {
	Entity** entities;
	u32 used;
	u32 size;
}EntityHandler;

void add_entity(Entity* e);
void init_entityhandler(EntityHandler* h, u32 initialSize);
void free_entityhandler();
void remove_entity(Entity* e);
void calc_all_rel_cam_pos(Player* player);
void sort_entities(Player* player);
void tick_item(Entity* item);
void tick_enemy(Entity* enemy);
void tick_bullet(Entity* bullet);
void check_entity_collisions(Player* p);
void free_and_remove_entity(Entity* e);