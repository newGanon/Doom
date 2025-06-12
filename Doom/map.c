#include "map.h"
#include "math.h"
#include "tex.h"

static Map* map;

void loadlevel(Map* map1) {
	map = map1;
	FILE* fp = NULL;
	fopen_s(&fp, "Levels/level4.txt", "r");
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
			sscanf_s(p, "%d %d %f %f", &sector->id, &sector->numWalls, &sector->zfloor, &sector->zceil);
			sector->id -= 1;
			sector->zfloor_old = sector->zfloor;
			if (sector->id == 0) {
				sector->index = 0;
			}
			else {
				Sector prec_sec = map->sectors[map->sectoramt - 2];
				sector->index = prec_sec.index + prec_sec.numWalls;
			}
		}
				   break;
		case WALL: {
			Wall* wall = &map->walls[map->wallnum++];
			sscanf_s(p, "%f %f %f %f %d", &wall->a.x, &wall->a.y, &wall->b.x, &wall->b.y, &wall->portal);
			wall->portal -= 1;
		}
				 break;
		case NONE:
			done = true;
			break;
		}
	}
	fclose(fp);
}


bool point_inside_sector(i32 sec, v2 p) {
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
void sort_walls(Player* p) {
	//calc distances
	for (i32 wallind = 0; wallind < map->wallnum; wallind++)
	{
		Wall wall = map->walls[wallind];
		v2 p1 = world_pos_to_camera(wall.a, *p);
		v2 p2 = world_pos_to_camera(wall.b, *p);

		map->walls[wallind].distance = (p1.y + p2.y) / 2;

	}
	//sort wall with distance from player
	for (i32 secind = 0; secind < map->sectoramt; secind++)
	{
		Sector sec = map->sectors[secind];
		for (i32 step = sec.index; step < sec.index + sec.numWalls; step++)
		{
			for (i32 i = sec.index; i < sec.index + sec.numWalls - 1; i++)
			{
				if (map->walls[i].distance > map->walls[i + 1].distance) {
					Wall tempWall = map->walls[i];
					map->walls[i] = map->walls[i + 1];
					map->walls[i + 1] = tempWall;
				}
			}
		}
	}
}

void trymove_player(Player* p) {
	//vertical collision detection
	const f32 gravity = -GRAVITY * SECONDS_PER_UPDATE;
	Sector* cur_sec = get_sector(p->sector);
	f32 eyeheight = p->sneak ? SNEAKHEIGHT : EYEHEIGHT;

	// player above ground
	if (cur_sec->zfloor < p->z - eyeheight && !p->in_air) {
		//p->inAir = true;
		p->z = eyeheight + cur_sec->zfloor;
	}
	// player below ground
	else if (cur_sec->zfloor > p->z - eyeheight) {
		p->z = eyeheight + cur_sec->zfloor;
	}
	// move down if player in ceiling
	if (cur_sec->zceil < p->z + HEADMARGIN) {
		p->z = cur_sec->zceil - HEADMARGIN;
	}

	// if not enough space in sector -> crushed
	if ((cur_sec->zceil - cur_sec->zfloor) < eyeheight + HEADMARGIN) {
		p->dead = true;
		return;
	}


	if (p->in_air) {
		p->velocity.z += gravity;
		f32 dvel = p->velocity.z * SECONDS_PER_UPDATE;
		//floor collision
		if (p->velocity.z < 0 && (p->z + dvel) < (cur_sec->zfloor + eyeheight)) {
			p->velocity.z = 0;
			p->in_air = false;
			p->z = (cur_sec->zfloor + eyeheight);
		}
		//ceiling collision
		else if (p->velocity.z > 0 && (p->z + dvel) > (cur_sec->zceil - HEADMARGIN)) {
			p->velocity.z = 0;
			p->z = cur_sec->zceil - HEADMARGIN;
		}
		//if no collision was detected just add the velocity
		else {
			p->z += dvel;
		}
	}
	
	//check for horizontal collision and if player entered new sector
	//TODO: fix hack that loops 2 times
	i32 wallind = -1;
	Sector* sec_old = cur_sec;
	bool in_air_old = p->in_air;
	f32 oldz = p->z;
	bool hit_portal = false;
	Sector* sec_new = sec_old;
	for (u8 t = 0; t < 2; t++) {
		for (i32 i = cur_sec->index; i < cur_sec->index + cur_sec->numWalls; i++) {
			Wall curwall = map->walls[i];
			if (BOXINTERSECT2D(p->pos.x, p->pos.y, p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) &&
				POINTSIDE2D(p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
					f32 stepl = curwall.portal >= 0 ? get_sector(curwall.portal)->zfloor : 10e10;
					f32 steph = curwall.portal >= 0 ? get_sector(curwall.portal)->zceil : -10e10;
					//collision with wall, top or lower part of portal
					if (stepl > p->z - eyeheight + STEPHEIGHT ||
						steph < p->z + HEADMARGIN ||
						(steph - stepl) < (eyeheight + HEADMARGIN) ||
						(p->sneak && !p->in_air && (cur_sec->zfloor - stepl) > 0.0f)) {
						//if player hit a corner set velocity to 0
						if (wallind != -1) {
							p->velocity.x = 0;
							p->velocity.y = 0;
							cur_sec = sec_old;
							p->sector = sec_old->id;
							p->in_air = in_air_old;
							p->z = oldz;
							break;
						}
						//collide with wall, project velocity vector onto wall vector
						v2 wallVec = { curwall.b.x - curwall.a.x, curwall.b.y - curwall.a.y };
						v2 projVel = {
							(p->velocity.x * wallVec.x + p->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.x,
							(p->velocity.x * wallVec.x + p->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.y
						};

						p->velocity.x = projVel.x;
						p->velocity.y = projVel.y;
						wallind = i;
					}
					//if player fits throught portal change playersector
					else if (curwall.portal >= 0) {
						if (hit_portal) {
							hit_portal = false;
							t = 2;
							break;
						}
						hit_portal = true;
						sec_new = get_sector(curwall.portal);
					}
			}
		}
		if (hit_portal) {
			t = 0;
			hit_portal = false;
			cur_sec = sec_new;
			p->sector = cur_sec->id;
			if (p->z < eyeheight + cur_sec->zfloor) p->z = eyeheight + cur_sec->zfloor;
			else if (p->z > eyeheight + cur_sec->zfloor) p->in_air = true;
		}
	}

	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;
}

bool trymove_entity(Entity* e, bool gravityactive) {
	Sector curSec = *get_sector(e->sector);

	if (gravityactive) {
		//vertical collision detection
		const f32 gravity = -GRAVITY;

		if (e->inAir) {
			e->velocity.z += gravity;
			f32 dvel = e->velocity.z;
			//floor collision
			if (e->velocity.z < 0 && (e->z + dvel) < (curSec.zfloor + e->scale.y)) {
				e->velocity.z = 0;
				e->inAir = 0;
				e->z = (curSec.zfloor + e->scale.y);
			}
			//ceiling collision
			else if (e->velocity.z > 0 && (e->z + dvel) > (curSec.zceil - HEADMARGIN)) {
				e->velocity.z = 0;
				e->z = curSec.zceil - HEADMARGIN;
			}
			//if no collision was detected just add the velocity
			else {
				e->z += dvel;
			}
		}
	}
	bool hit = false;
	for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
		Wall* curwall = &map->walls[i];
		v2 intersection;
		v2 pos = e->pos;
		if ((POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0) &&
			(get_line_intersection(pos, (v2) { pos.x + e->velocity.x, pos.y + e->velocity.y }, curwall->a, curwall->b, & intersection))) {
			f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10;
			f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10;
			if (e->type == Projectile) {
				v2 mappos = (v2){ intersection.x - curwall->a.x, intersection.y - curwall->a.y };
				v2 wallpos = (v2){ sqrt(mappos.x * mappos.x + mappos.y * mappos.y), e->z };
				if (spawn_decal(wallpos, curwall, (v2) {2.0f, 2.0f}, 1)) {
					hit = true;
					break;
				}
			}
			//collision with wall, top or lower part of portal
			else if (stepl > e->z - e->scale.y + STEPHEIGHT ||
				steph < e->z + HEADMARGIN) {
				//collide with wall, project velocity vector onto wall vector
				v2 wallVec = { curwall->b.x - curwall->a.x, curwall->b.y - curwall->a.y };
				v2 projVel = {
					(e->velocity.x * wallVec.x + e->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.x,
					(e->velocity.x * wallVec.x + e->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.y
				};

				e->velocity.x = projVel.x;
				e->velocity.y = projVel.y;
				hit = true;
				break;
			}

			//if entity fits throught portal change entitysector
			if (curwall->portal >= 0) {
				curSec = map->sectors[curwall->portal];
				e->sector = curSec.id;
				if (e->type == Projectile) continue;
				if (e->z < e->scale.y + curSec.zfloor) e->z = e->scale.y + curSec.zfloor;
				else if (e->z > e->scale.y + curSec.zfloor) e->inAir = 1;
			}
		}
	}

	e->pos.x += e->velocity.x;
	e->pos.y += e->velocity.y;

	return hit;
}
Sector* get_sector(i32 index) {
	if (index > map->sectoramt) return NULL;
	else return &map->sectors[index];
}

Wall* get_wall(i32 index) {
	if (index > map->wallnum) return NULL;
	else return &map->walls[index];
}

i32 get_sectoramt() {
	return map->sectoramt;
}

Decal* spawn_decal(v2 wallpos, Wall* curwall, v2 size, i32 tex_id) {
	f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10;
	f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10;

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
	if (type == NONE) return NULL;


	Decal* decal = malloc(sizeof(Decal));
	if (decal) {
		decal->tex_num = 1;
		decal->next = NULL;
		decal->prev = NULL;
		decal->size = size;
		decal->wall_type = type;
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


bool move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up) {
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


f32 get_relative_decal_wall_height(Decal* d, Wall* wall, f32 cur_sec_floorz) {
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
			Sector* sec = get_sector(wall->portal);
			rel_decal_height = (d->wallpos.y + sec->zfloor) - cur_sec_floorz;
			break;
		}
		// decal has height relative to ceil of portal sector
		case PORTAL_UPPER: {
			Sector* sec = get_sector(wall->portal);
			rel_decal_height = d->wallpos.y + sec->zceil;
			break;
		}
	}
	return rel_decal_height;
}


RaycastResult raycast(Sector* cursec, v2 pos, v2 target_pos, f32 z) {
	if(cursec->zceil < z || cursec->zfloor > z) return (RaycastResult) { .hit = false };
	v2 intersection;
	bool hit = false;
	for (i32 i = cursec->index; i < cursec->index + cursec->numWalls; i++) {
		Wall* curwall = &map->walls[i];
		i32 test = (POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y));
		if ((POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0) && 
			(get_line_intersection(pos, target_pos, curwall->a, curwall->b, &intersection))) {
			// normal wall hit
			if (curwall->portal == -1) { hit = true; }
			// portal hit
			else {
				f32 stepl = get_sector(curwall->portal)->zfloor;
				f32 steph = get_sector(curwall->portal)->zceil;
				// top or bottom of portal hit
				if (stepl > z || steph < z) { hit = true; }
				// fit through portal, change sector
				else {
					cursec = get_sector(curwall->portal);
					i = cursec->index;
					pos = intersection;
				}
			}
			// if hit was detected return the result
			if (hit) {
				v2 hit_pos = (v2){ intersection.x - curwall->a.x, intersection.y - curwall->a.y };
				v2 pos_to_hit = (v2){ intersection.x - pos.x, intersection.y - pos.y };
				f32 distance = sqrt(pos_to_hit.x * pos_to_hit.x + pos_to_hit.y * pos_to_hit.y);
				return (RaycastResult) { .hit = true, .wall_pos = hit_pos, .wall = curwall, .wall_sec = cursec, .distance = distance };
			}
		}
	}
	return (RaycastResult) { .hit = false };
}