#include "map.h"
#include "math.h"
#include "tex.h"

Map* map;

void spawn_decal(v2 wallpos, f32 floor_height, f32 ceil_height, Wall* curwall, f32 decal_height, wall_section_type type);

void load_level(Map* map1) {
	map = map1;
	FILE* fp = NULL;
	fopen_s(&fp, "level4.txt", "r");
	ASSERT(fp, "error opening leveldata file");
	enum { SECTOR, WALL, NONE } sm = NONE;
	u8 done = 0;

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
			Sector* sector = &map->sectors[map->sectornum++];
			sscanf_s(p, "%d %d %d %f %f", &sector->id, &sector->index, &sector->numWalls, &sector->zfloor, &sector->zceil);
			sector->id -= 1;
		}
				   break;
		case WALL: {
			Wall* wall = &map->walls[map->wallnum++];
			sscanf_s(p, "%f %f %f %f %d", &wall->a.x, &wall->a.y, &wall->b.x, &wall->b.y, &wall->portal);
			wall->portal -= 1;
		}
				 break;
		case NONE:
			done = 1;
			break;
		}
	}
	fclose(fp);
}


u8 point_inside_sector(i32 sec, v2 p) {
	Sector s = map->sectors[sec];
	for (i32 i = s.index; i < (s.index + s.numWalls); i++) {
		Wall w = map->walls[i];
		if (POINTSIDE2D(p.x, p.y, w.a.x, w.a.y, w.b.x, w.b.y) > 0) {
			return 0;
		}
	}
	return 1;
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
	for (i32 secind = 0; secind < map->sectornum; secind++)
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
	const f32 gravity = -GRAVITY * FRAMETICKS;
	Sector* curSec = get_sector(p->sector);;

	if (p->inAir) {
		p->velocity.z += gravity;
		f32 dvel = p->velocity.z * FRAMETICKS;
		//floor collision
		if (p->velocity.z < 0 && (p->z + dvel) < (curSec->zfloor + EYEHEIGHT)) {
			p->velocity.z = 0;
			p->inAir = 0;
			p->z = (curSec->zfloor + EYEHEIGHT);
		}
		//ceiling collision
		else if (p->velocity.z > 0 && (p->z + dvel) > (curSec->zceil - HEADMARGIN)) {
			p->velocity.z = 0;
			p->z = curSec->zceil - HEADMARGIN;
		}
		//if no collision was detected just add the velocity
		else {
			p->z += dvel;
		}
	}
	
	//check for horizontal collision and if player entered new sector
	//TODO: fix hack that loops 2 times
	i32 wallind = -1;
	Sector* oldSec = curSec;
	u8 oldinAir = p->inAir;
	f32 oldz = p->z;
	u8 hitPortal = 0;
	Sector* newSec = oldSec;
	for (u8 t = 0; t < 2; t++) {
		for (i32 i = curSec->index; i < curSec->index + curSec->numWalls; i++) {
			Wall curwall = map->walls[i];
			if (BOXINTERSECT2D(p->pos.x, p->pos.y, p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) &&
				POINTSIDE2D(p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
					f32 stepl = curwall.portal >= 0 ? map->sectors[curwall.portal].zfloor : 10e10;
					f32 steph = curwall.portal >= 0 ? map->sectors[curwall.portal].zceil : -10e10;
					//collision with wall, top or lower part of portal
					if (stepl > p->z - EYEHEIGHT + STEPHEIGHT ||
						steph < p->z + HEADMARGIN) {
						//if player hit a corner set velocity to 0
						if (wallind != -1) {
							p->velocity.x = 0;
							p->velocity.y = 0;
							curSec = oldSec;
							p->sector = oldSec->id;
							p->inAir = oldinAir;
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
						if (hitPortal == 1) {
							hitPortal = 0;
							t = 2;
							break;
						}
						hitPortal = 1;
						newSec = get_sector(curwall.portal);
					}
			}
		}
		if (hitPortal) {
			t = 0;
			hitPortal = 0;
			curSec = newSec;
			p->sector = curSec->id;
			if (p->z < EYEHEIGHT + curSec->zfloor) p->z = EYEHEIGHT + curSec->zfloor;
			else if (p->z > EYEHEIGHT + curSec->zfloor) p->inAir = 1;
		}
	}

	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;
}

u8 trymove_entity(Entity* e, u8 gravityactive) {
	Sector curSec = *get_sector(e->sector);;

	if (gravityactive) {
		//vertical collision detection
		const f32 gravity = -GRAVITY * FRAMETICKS;

		if (e->inAir) {
			e->velocity.z += gravity;
			f32 dvel = e->velocity.z * FRAMETICKS;
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
	u8 hit = 0;
	for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
		Wall* curwall = &map->walls[i];
		v2 intersection;
		v2 pos = e->pos;
		if ((POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0) &&
			(get_line_intersection(pos, (v2) { pos.x + e->velocity.x, pos.y + e->velocity.y }, curwall->a, curwall->b, & intersection))) {
			f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10;
			f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10;
			if (e->type == Projectile) {
				v2 pos = (v2){ intersection.x - curwall->a.x, intersection.y - curwall->a.y };
				//collision with wall
				wall_section_type type = NONE;
				if (curwall->portal == -1) {
					type = WALL;
				}
				// collision lower part of portal
				else if (stepl > e->z - e->scale.y / 2.0f) {
					type = PORTAL_LOWER;
				} 
				// collision with upper part of portal
				else if (steph < (e->z + e->scale.y / 2.0f) && steph < curSec.zceil) {
					type = PORTAL_UPPER;
				}

				if (type != NONE) {
					spawn_decal(pos, curSec.zfloor, curSec.zceil, curwall, e->z, type);
					hit = 1;
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
				hit = 1;
				break;
			}

			//if entity fits throught portal change playersector
			if (curwall->portal >= 0) {
				curSec = map->sectors[curwall->portal];
				e->sector = curSec.id;;
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
	if (index > map->sectornum) return NULL;
	else return &map->sectors[index];
}

Wall* get_wall(i32 index) {
	if (index > map->wallnum) return NULL;
	else return &map->walls[index];
}

i32 get_sectornum() {
	return map->sectornum;
}

void spawn_decal(v2 wallpos, f32 floor_height, f32 ceil_height, Wall* curwall, f32 height, wall_section_type type) {
	if ((type == PORTAL_LOWER || type == PORTAL_UPPER) && curwall->portal == -1) return;
	f32 len = sqrt(wallpos.x * wallpos.x + wallpos.y * wallpos.y);
	f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10;
	f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10;
	Decal* decal = malloc(sizeof(Decal));
	if (decal) {
		decal->tex = get_texture(1);
		decal->next = NULL;
		decal->prev = NULL;
		decal->size = (v2){ 2.0f, 2.0f };
		decal->wall_type = type;
		f32 decal_height = 0.0f;
		switch (type) {
			// decal has absoute height
			case WALL: {
				decal_height = height;
				break;
			}
			// decal has height relative to floor of portal sector
			case PORTAL_LOWER: {
				decal_height = height - stepl;
				break;
			}
			// decal has height relative to ceil of portal sector
			case PORTAL_UPPER: {
				decal_height = height - steph;
				break;
			}
		}
		// offset position a little as the wallpos is the bottom left corner of the decal
		decal->wallpos = (v2){ len - (decal->size.x / 2.0f), decal_height - (decal->size.y / 2.0f)};

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
	}
}

u8 check_hitscan_collsion(Player* p) {
	Sector curSec = *get_sector(p->sector);
	v2 intersection;
	v2 pos = p->pos;
	for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
		Wall* curwall = &map->walls[i];
		if ((POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0) &&
			(get_line_intersection(pos, (v2) { pos.x + p->anglecos * 1000.0f, pos.y + p->anglesin * 1000.0f}, curwall->a, curwall->b, &intersection))) {
			f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10;
			f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10;
			//collision with wall, top or lower part of portal
			if (stepl > p->z || steph < p->z) {
				//spawn_decal(intersection, curSec.zfloor, curSec.zceil, curwall, p->z);
				break;
			}
			//if hitscan projectile fits throught portal change sector
			else if (curwall->portal >= 0) {
				curSec = map->sectors[curwall->portal];
				i = curSec.index;
				pos = intersection;
			}
		}
	}
}






bool move_sector_plane(Sector* sec, f32 speed, f32 dest, bool floor, bool up) {
	i32 dir = up ? 1 : -1;
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
