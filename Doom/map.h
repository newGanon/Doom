#pragma once
#include "util.h"
#include "player.h"
#include "entity.h"
#include "draw.h"

typedef struct Decal {
	struct Decal* next;
	struct Decal* prev;
	i32 tag;
	Texture* tex;
	v2 wallpos; // bottom left corner of decal
	v2 size;  // width and height of the decal
} Decal;

typedef struct Sector {
	i32 id, index, numWalls;
	f32 zfloor, zceil;
	i32 tag;
	void* specialdata; 	// pointer to ticker to reverse actions
} Sector;

typedef struct Wall {
	Decal* decalhead;
	v2 a, b;
	i32 portal;
	f32 distance;
	bool tex_floor_anchored; // if texture should get drawn starting from floor or ceil, gets switched when ceil or floor moves 
} Wall;

typedef struct Map {
	Wall walls[WALL_MAX];
	i32 wallnum;
	Sector sectors[SECTOR_MAX];
	i32 sectornum;
} Map;


void load_level(Map* map);
u8 point_inside_sector(i32 sec, v2 p); 
void sort_walls(Player* player);
void trymove_player(Player* p);
u8 trymove_entity(Entity* e, u8 gravityactive);
Sector* get_sector(i32 index);
Wall* get_wall(i32 index);
i32 get_sectornum();
u8 check_hitscan_collsion(Player* p);

bool move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up);
void set_sector_wall_draw_anchor(Sector* sec, bool tex_floor_anchored);