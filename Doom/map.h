#pragma once
#include "util.h"

typedef enum plat_type_e plat_type;

typedef enum wall_section_type {
	WALL,
	PORTAL_LOWER,
	PORTAL_UPPER,
	NONE
} wall_section_type;

typedef struct Decal_s {
	struct Decal_s* next;
	struct Decal_s* prev;
	i32 tag;
	plat_type tag_action;
	wall_section_type wall_type;
	u32 tex_num;
	v2 wallpos; // bottom left corner of decal
	v2 size;  // width and height of the decal
	bool front;
} Decal;

typedef struct Sector_s {
	i32 id, index, numWalls;
	f32 zfloor, zceil;
	f32 zfloor_old; // first value of the zfloor
	i32 tag;
	i32 lightlevel;
	void* specialdata; // pointer to ticker to reverse actions
} Sector;

typedef struct Wall_s {
	Decal* decalhead;
	v2 a, b;
	i32 portal;
	f32 distance;
	bool transparent;
	u32 tex;
} Wall;

typedef struct Map_s {
	Wall walls[WALL_MAX];
	i32 wallnum;
	Sector sectors[SECTOR_MAX];
	i32 sectoramt;
} Map;


typedef struct RaycastResult_s {
	bool hit;
	bool front;
	v2 wall_pos;
	f32 distance;
	Wall* wall;
	Sector* wall_sec;
}RaycastResult;


void map_init(Map* map);
Sector* map_get_sector_by_idx(i32 index);
Sector* map_get_sector_by_idxx(i32 id);
Wall* map_get_wall(i32 index);
i32 map_get_sectoramt();
bool map_point_inside_sector(i32 sec, v2 p); 
void map_sort_walls(v2 cam_pos, f32 camsin, f32 camcos);
f32 map_decal_wall_height(Decal* d, Wall* wall, f32 cur_sec_floorz);
Decal* map_spawn_decal(v2 wallpos, Wall* curwall, v2 size, i32 tex_id, bool front);
RaycastResult map_raycast(Sector* cursec, v2 pos, v2 target_pos, f32 z);
bool map_move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up);

