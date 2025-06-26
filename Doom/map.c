#include "map.h"
#include "math.h"
#include "tex.h"
#include <ctype.h>

static Map* map;

void map_init(Map* map1) {
	map = map1;
	FILE* fp = NULL;
	fopen_s(&fp, "Levels/save.txt", "r");
	ASSERT(fp, "error opening leveldata file");
	enum { SECTOR, WALL, NONE } sm = NONE;
	bool done = false;

	char line[1024];
	while (fgets(line, sizeof(line), fp) && !done) {
		char* p = line;
		while (isspace(*p)) { p++; }
		if (!*p || *p == '#') continue;
		if (*p == '{') {
			p++;
			if (*p == 'S') { sm = SECTOR; continue; }
			if (*p == 'W') { sm = WALL; continue; }
			if (*p == 'E')  sm = NONE;
			else sm = NONE;
		}
		switch (sm) {
		case SECTOR: {
			Sector* sector = &map->sectors[map->sectoramt++];
			sscanf_s(p, "%d %d %f %f %d %d", &sector->id, &sector->numWalls, &sector->zfloor, &sector->zceil, &sector->lightlevel, &sector->tag);
			//sector->id -= 1;
			sector->zfloor_old = sector->zfloor;
			if (sector->id == 0) {
				sector->index = 0;
			}
			else {
				// calculate wall index
				Sector prec_sec = map->sectors[map->sectoramt - 2];
				sector->index = prec_sec.index + prec_sec.numWalls;
			}
		}
				   break;
		case WALL: {
			Wall* wall = &map->walls[map->wallnum++];
			i32 front;
			sscanf_s(p, "%f %f %f %f %d %d", &wall->a.x, &wall->a.y, &wall->b.x, &wall->b.y, &wall->portal, &front);
			//wall->portal -= 1;
			wall->tex = WALLTEXTURE;

			// TODO: REMOVE
			// make 2 walls transparent
			//if (map->wallnum == 9 || map->wallnum == 10) {
			//	wall->transparent = true;
			//	wall->tex = 3;
			//}
		}
				 break;
		case NONE:
			done = true;
			break;
		}
	}
	fclose(fp);
}


bool map_point_inside_sector(i32 sec, v2 p) {
	Sector s = map->sectors[sec];
	for (i32 i = s.index; i < (s.index + s.numWalls); i++) {
		Wall w = map->walls[i];
		if (POINTSIDE2D(p.x, p.y, w.a.x, w.a.y, w.b.x, w.b.y) > 0) {
			return false;
		}
	}
	return true;
}

// walls are sorted in order to to draw them recursivly and allow for non konvex sectors
void map_sort_walls(v2 cam_pos, f32 camsin, f32 camcos) {
	//calc distances
	for (i32 wallind = 0; wallind < map->wallnum; wallind++) {
		Wall* wall = map_get_wall(wallind);
		// calc distance to camera
		f32 mid_x = (wall->a.x + wall->b.x) / 2.0f;
		f32 mid_y = (wall->a.y + wall->b.y) / 2.0f;
		f32 dx = mid_x - cam_pos.x;
		f32 dy = mid_y - cam_pos.y;
		f32 dis = dx * dx + dy * dy;
		map->walls[wallind].distance = dis;
	}
	//sort wall with distance from player 
	// TODO: implement more efficient sorting algorithm
	for (i32 secind = 0; secind < map->sectoramt; secind++) {
		Sector sec = map->sectors[secind];
		for (i32 step = sec.index; step < sec.index + sec.numWalls; step++) {
			for (i32 i = sec.index; i < sec.index + sec.numWalls - 1; i++) {
				Wall* w1 = map_get_wall(i);
				Wall* w2 = map_get_wall(i+1);
				if (w1->distance > w2->distance) {
					Wall tempWall = *w1;
					map->walls[i] = map->walls[i + 1];
					map->walls[i + 1] = tempWall;
				}
			}
		}
	}
}

Sector* map_get_sector_by_idx(i32 index) {
	if (index > map->sectoramt) return NULL;
	else return &map->sectors[index];
}

Sector* map_get_sector_by_idxx(i32 index) {
	if (index > map->sectoramt) return NULL;
	else return &map->sectors[index];
}

Sector* map_get_sector_by_id(i32 id) {
	for (i32 i = 0; i < map->sectoramt; i++) {
		Sector* sec = &map->sectors[i];
		if (sec->id == id) {
			return sec;
		}
	}
	return NULL;
}

Wall* map_get_wall(i32 index) {
	if (index > map->wallnum) return NULL;
	else return &map->walls[index];
}

i32 map_get_sectoramt() {
	return map->sectoramt;
}

wall_section_type map_get_walltype_from_position(v2 wallpos, Wall* curwall, f32 stepl, f32 steph) {
	wall_section_type type = NONE;
	if (curwall->portal == -1) {
		type = WALL;
	}
	// collision lower part of portal
	else if (stepl > wallpos.y) {
		type = PORTAL_LOWER;
	}
	// collision with upper part of portal
	else if (steph < wallpos.y) {
		type = PORTAL_UPPER;
	}
	return type;
}


Decal* map_spawn_decal(v2 wallpos, Wall* curwall, v2 size, i32 tex_id, bool front) {
	
	f32 stepl = curwall->portal >= 0 ? map_get_sector_by_idxx(curwall->portal)->zfloor : 10e10f;
	f32 steph = curwall->portal >= 0 ? map_get_sector_by_idxx(curwall->portal)->zceil : -10e10f;
	wall_section_type type = map_get_walltype_from_position(wallpos, curwall, stepl, steph);
	if (type == NONE) return NULL;
	Decal* decal = malloc(sizeof(Decal));
	if (decal) {
		decal->tex_num = tex_id;
		decal->next = NULL;
		decal->prev = NULL;
		decal->size = size;
		decal->wall_type = type;
		decal->front = front;
		f32 decal_height = 0.0f;
		switch (type) {
			// decal has absoute height
			case WALL: {
				decal_height = wallpos.y;
				break;
			}
			// decal has height relative to floor of portal sector
			case PORTAL_LOWER: {
				decal_height = wallpos.y - stepl;
				break;
			}
			// decal has height relative to ceil of portal sector
			case PORTAL_UPPER: {
				decal_height = wallpos.y - steph;
				break;
			}
		}
		// offset position a little as the wallpos is the bottom left corner of the decal
		decal->wallpos = (v2){ wallpos.x - (decal->size.x / 2.0f), decal_height - (decal->size.y / 2.0f)};

		// add the decal to the decal linked list of the wall
		if (curwall->decalhead == NULL) {
			curwall->decalhead = decal;
		}
		else {
			Decal* curdecal = curwall->decalhead;
			while (curdecal->next != NULL) curdecal = curdecal->next;
			curdecal->next = decal;
			decal->prev = curdecal;
		}
		return decal;
	}
	return NULL;
}


bool map_move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up) {
	i32 dir = up ? 1 : -1;
	speed = speed * SECONDS_PER_UPDATE;
	f32 new_height;
	if (floor) {
		new_height = (speed * dir) + sec->zfloor;
		if (up) {
			if (new_height > dest) {
				sec->zfloor = dest;
				return true;
			} 
			else {
				sec->zfloor = new_height;
				return false;
			}
		}
		// down
		else {
			if (new_height < dest) {
				sec->zfloor = dest;
				return true;
			}
			else {
				sec->zfloor = new_height;
				return false;
			}
		}
	}
	// ceil
	else {
		new_height = (speed * dir) + sec->zceil;
		if (up) {
			if (new_height > dest) {
				sec->zceil = dest;
				return true;
			}
			else {
				sec->zceil = new_height;
				return false;
			}
		}
		// down
		else {
			if (new_height < dest) {
				sec->zceil = dest;
				return true;
			}
			else {
				sec->zceil = new_height;
				return false;
			}
		}
	}


	return false;
}


f32 map_decal_wall_height(Decal* d, Wall* wall, f32 cur_sec_floorz) {
	f32 rel_decal_height = 0.0f;
	// if decal is on the upper part of portal, then subtract neightbouring sector ceiling height, to get absolute position of decal on wall
	switch (d->wall_type) {
		// decal has absoute height
		case WALL: {
			rel_decal_height = d->wallpos.y - cur_sec_floorz;
			break;
		}
		// decal has height relative to floor of portal sector
		case PORTAL_LOWER: {
			Sector* sec = map_get_sector_by_idxx(wall->portal);
			rel_decal_height = (d->wallpos.y + sec->zfloor) - cur_sec_floorz;
			break;
		}
		// decal has height relative to ceil of portal sector
		case PORTAL_UPPER: {
			Sector* sec = map_get_sector_by_idxx(wall->portal);
			rel_decal_height = d->wallpos.y + sec->zceil;
			break;
		}
	}
	return rel_decal_height;
}


RaycastResult map_raycast(Sector* cursec, v2 pos, v2 target_pos, f32 z) {
	if(cursec->zceil < z || cursec->zfloor > z) return (RaycastResult) { .hit = false };
	v2 intersection;
	bool hit = false;
	for (i32 i = cursec->index; i < cursec->index + cursec->numWalls; i++) {
		Wall* curwall = &map->walls[i];
		bool front = (POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0);
		if (get_line_intersection(pos, target_pos, curwall->a, curwall->b, &intersection)) {
			// raycast cant hit transarent walls
			if (curwall->transparent) continue;
			// normal wall hit
			else if (curwall->portal == -1) { hit = true; }
			// portal hit
			else {
				f32 stepl = map_get_sector_by_idxx(curwall->portal)->zfloor;
				f32 steph = map_get_sector_by_idxx(curwall->portal)->zceil;
				// top or bottom of portal hit
				if (stepl > z || steph < z) { hit = true; }
				// fit through portal, change sector
				else {
					if (POINTSIDE2D(target_pos.x, target_pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) > 0) {
						cursec = map_get_sector_by_idxx(curwall->portal);
						i = cursec->index;
						pos = intersection;
					}
				}
			}
			// if hit was detected return the result
			if (hit) {
				v2 hit_pos = (v2){ intersection.x - curwall->a.x, intersection.y - curwall->a.y };
				v2 pos_to_hit = (v2){ intersection.x - pos.x, intersection.y - pos.y };
				f32 distance = sqrtf(pos_to_hit.x * pos_to_hit.x + pos_to_hit.y * pos_to_hit.y);
				return (RaycastResult) { .hit = true, .wall_pos = hit_pos, .wall = curwall, .wall_sec = cursec, .distance = distance, .front = front };
			}
		}
	}
	return (RaycastResult) { .hit = false };
}