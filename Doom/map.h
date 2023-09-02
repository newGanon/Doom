#pragma once
#include "util.h"
#include "player.h"
#include "entity.h"

typedef struct Sector {
	i32 id, index, numWalls;
	f32 zfloor, zceil;
}Sector;

typedef struct Wall {
	v2 a, b;
	i32 portal;
	f32 distance;
}Wall;

typedef struct Map {
	Wall walls[WALL_MAX];
	i32 wallnum;
	Sector sectors[SECTOR_MAX];
	i32 sectornum;
} Map;


void loadLevel(Map* map);
u8 pointInsideSector(i32 sec, v2 p); 
void sortWalls(Player* player);
void trymove_player(Player* p);
u8 trymove_entity(Entity* e, u8 gravityactive);
Sector* get_sector(i32 index);
Wall* get_wall(i32 index);
i32 get_sectornum();
