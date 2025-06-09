#pragma once
#include "util.h"
#include "player.h"
#include "entity.h"
#include "draw.h"

typedef enum wall_section_type {
	WALL,
	PORTAL_LOWER,
	PORTAL_UPPER,
	NONE
} wall_section_type;

typedef struct Decal {
	struct Decal* next;
	struct Decal* prev;
	i32 tag;
	wall_section_type wall_type;
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
} Wall;

typedef struct Map {
	Wall walls[WALL_MAX];
	i32 wallnum;
	Sector sectors[SECTOR_MAX];
	i32 sectoramt;
} Map;


typedef struct RaycastResult {
	bool hit;
	v2 wall_pos;
	Wall* wall;
	Sector* wall_sec;
}RaycastResult;


void load_level(Map* map);
u8 point_inside_sector(i32 sec, v2 p); 
void sort_walls(Player* player);
void trymove_player(Player* p);
u8 trymove_entity(Entity* e, u8 gravityactive);
Sector* get_sector(i32 index);
Wall* get_wall(i32 index);
i32 get_sectoramt();
f32 get_relative_decal_wall_height(Decal* d, Wall* wall, f32 cur_sec_floorz);

Decal* spawn_decal(v2 wallpos, Wall* curwall, v2 size, i32 tex_id);

RaycastResult raycast(Sector* cursec, v2 pos, v2 target_pos, f32 z);

bool move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up);

