#include "map.h"
#include "math.h"
#include "tex.h"

Map* map;

void loadLevel(Map* map1) {
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
		}
				   break;
		case WALL: {
			Wall* wall = &map->walls[map->wallnum++];
			sscanf_s(p, "%f %f %f %f %d", &wall->a.x, &wall->a.y, &wall->b.x, &wall->b.y, &wall->portal);
			//wall->decalhead = NULL;
		}
				 break;
		case NONE:
			done = 1;
			break;
		}
	}
	fclose(fp);
}



u8 pointInsideSector(i32 sec, v2 p) {
	Sector s = map->sectors[sec - 1];
	for (i32 i = s.index; i < (s.index + s.numWalls); i++) {
		Wall w = map->walls[i];
		if (POINTSIDE2D(p.x, p.y, w.a.x, w.a.y, w.b.x, w.b.y) > 0) {
			return 0;
		}
	}
	return 1;
}

void sortWalls(Player* p) {
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
	Sector curSec = map->sectors[p->sector - 1];

	if (p->inAir) {
		p->velocity.z += gravity;
		f32 dvel = p->velocity.z * FRAMETICKS;
		//floor collision
		if (p->velocity.z < 0 && (p->z + dvel) < (curSec.zfloor + EYEHEIGHT)) {
			p->velocity.z = 0;
			p->inAir = 0;
			p->z = (curSec.zfloor + EYEHEIGHT);
		}
		//ceiling collision
		else if (p->velocity.z > 0 && (p->z + dvel) > (curSec.zceil - HEADMARGIN)) {
			p->velocity.z = 0;
			p->z = curSec.zceil - HEADMARGIN;
		}
		//if no collision was detected just add the velocity
		else {
			p->z += dvel;
		}
	}
	v2 intersection;
	for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
		Wall curwall = map->walls[i];
		if ((POINTSIDE2D(p->pos.x, p->pos.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) <= 0) &&
			(get_line_intersection(p->pos, (v2) {p->pos.x + p->velocity.x, p->pos.y + p->velocity.y}, curwall.a, curwall.b, &intersection))) {
				f32 stepl = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zfloor : 10e10;
				f32 steph = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zceil : -10e10;
				if (stepl > p->z - EYEHEIGHT + STEPHEIGHT ||
					steph < p->z + HEADMARGIN) {
					if (fabs(p->velocity.x) + fabs(p->velocity.y) < 0.2) return;
					v2 velnorm = v2Normalize((v2) { p->velocity.x, p->velocity.y });
					v2 newPos = (v2){ intersection.x - (velnorm.x * 0.01f), intersection.y - (velnorm.y * 0.01f)};

					v2 posDiff = v2Sub(newPos, p->pos);
					p->pos = newPos;
					p->velocity.x = p->velocity.x - posDiff.x;
					p->velocity.y = p->velocity.y - posDiff.y;

					//collide with wall, project velocity vector onto wall vector
					v2 wallVec = { curwall.b.x - curwall.a.x, curwall.b.y - curwall.a.y };
					v2 projVel = {
						(p->velocity.x * wallVec.x + p->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.x,
						(p->velocity.x * wallVec.x + p->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.y
					};
					p->velocity.x = projVel.x;
					p->velocity.y = projVel.y;
					i = curSec.index - 1;			
				}
				else if (curwall.portal > 0) {
					curSec = map->sectors[curwall.portal - 1];
					i = curSec.index - 1;
					p->sector = curSec.id;
					if (p->z < EYEHEIGHT + curSec.zfloor) p->z = EYEHEIGHT + curSec.zfloor;
					else if (p->z > EYEHEIGHT + curSec.zfloor) p->inAir = 1;
				}
			}
	}
	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;

	
	/*
	//check for horizontal collision and if player entered new sector
	//TODO: fix hack that loops 2 times
	i32 wallind = -1;
	Sector oldSec = curSec;
	u8 oldinAir = p->inAir;
	f32 oldz = p->z;
	u8 hitPortal = 0;
	Sector newSec;
	for (u8 t = 0; t < 2; t++) {
		for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
			Wall curwall = map->walls[i];
			if (BOXINTERSECT2D(p->pos.x, p->pos.y, p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) &&
				POINTSIDE2D(p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
					f32 stepl = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zfloor : 10e10;
					f32 steph = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zceil : -10e10;
					//collision with wall, top or lower part of portal
					if (stepl > p->z - EYEHEIGHT + STEPHEIGHT ||
						steph < p->z + HEADMARGIN) {
						//if player hit a corner set velocity to 0
						if (wallind != -1) {
							p->velocity.x = 0;
							p->velocity.y = 0;
							curSec = oldSec;
							p->sector = oldSec.id;
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
					else if (curwall.portal > 0) {
						if (hitPortal == 1) {
							hitPortal = 0;
							t = 2;
							break;
						}
						hitPortal = 1;
						newSec = map->sectors[curwall.portal - 1];
					}
			}
		}
		if (hitPortal) {
			t = 0;
			hitPortal = 0;
			curSec = newSec;
			p->sector = curSec.id;
			if (p->z < EYEHEIGHT + curSec.zfloor) p->z = EYEHEIGHT + curSec.zfloor;
			else if (p->z > EYEHEIGHT + curSec.zfloor) p->inAir = 1;
		}
	}

	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;
	*/
}

u8 trymove_entity(Entity* e, u8 gravityactive) {
	Sector curSec = map->sectors[e->sector - 1];

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
		Wall curwall = map->walls[i];
		if (BOXINTERSECT2D(e->pos.x, e->pos.y, e->pos.x + e->velocity.x, e->pos.y + e->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) &&
			POINTSIDE2D(e->pos.x + e->velocity.x, e->pos.y + e->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
				f32 stepl = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zfloor : 10e10;
				f32 steph = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zceil : -10e10;
				//collision with wall, top or lower part of portal
				if (stepl > e->z - e->scale.y + STEPHEIGHT ||
					steph < e->z + HEADMARGIN) {
					//collide with wall, project velocity vector onto wall vector
					v2 wallVec = { curwall.b.x - curwall.a.x, curwall.b.y - curwall.a.y };
					v2 projVel = {
						(e->velocity.x * wallVec.x + e->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.x,
						(e->velocity.x * wallVec.x + e->velocity.y * wallVec.y) / (wallVec.x * wallVec.x + wallVec.y * wallVec.y) * wallVec.y
					};
					e->velocity.x = projVel.x;
					e->velocity.y = projVel.y;
					hit = 1;
				}
				//if player fits throught portal change playersector
				else if (curwall.portal > 0) {
					curSec = map->sectors[curwall.portal - 1];
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

u8 check_hitscan_collsion(Player* p) {
	Sector curSec = map->sectors[p->sector - 1];
	v2 intersection;
	v2 pos = p->pos;
	for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
		Wall* curwal = &map->walls[i];
		if ((POINTSIDE2D(pos.x, pos.y, curwal->a.x, curwal->a.y, curwal->b.x, curwal->b.y) < 0) &&
			(get_line_intersection(pos, (v2) { pos.x + p->anglecos * 1000.0f, pos.y + p->anglesin * 1000.0f}, curwal->a, curwal->b, &intersection))) {
				f32 stepl = curwal->portal > 0 ? map->sectors[curwal->portal - 1].zfloor : 10e10;
				f32 steph = curwal->portal > 0 ? map->sectors[curwal->portal - 1].zceil : -10e10;
				//collision with wall, top or lower part of portal
				if (stepl > p->z || steph < p->z) {
					//collide with wall, project velocity vector onto wall vector
					printf("X:%f, Y:%f \n", intersection.x, intersection.y);
					v2 wallposvec = { intersection.x - curwal->a.x, intersection.y - curwal->a.y };
					f32 len = sqrt(wallposvec.x * wallposvec.x + wallposvec.y * wallposvec.y);
					

					Decal* decal = malloc(sizeof(Decal));
					if (decal) {
						decal->tex = get_texture(1);
						//todo when changing walltilsize this has to bve changed too
						f32 width = 2.0f;
						f32 height = 4.0f;
						if (steph < p->z) {
							decal->offset = (v2i){ ((i32)(len / width * 256) - 64), ((curSec.zceil - p->z) * 256 / height - 64) };
							decal->onportalbottom = 0;
						}
						else {
							decal->offset = (v2i){ ((i32)(len / width * 256) - 64), ((map->sectors[curwal->portal - 1].zfloor - p->z) * 256 / height - 64) };
							decal->onportalbottom = 1;
						}
						decal->next = NULL;
						decal->prev = NULL;
						decal->scale = 0.5f;
						decal->scaledsize = (v2i){ decal->tex->width * decal->scale, decal->tex->height * decal->scale };

						if (curwal->decalhead == NULL) {
							curwal->decalhead = decal;
						}
						else {
							Decal* curdecal = curwal->decalhead;
							while (curdecal->next != NULL) curdecal = curdecal->next;
							curdecal->next = decal;
							decal->prev = curdecal;
						}
					}
					break;
				}
				//if player fits throught portal change playersector
				else if (curwal->portal > 0) {
					curSec = map->sectors[curwal->portal - 1];
					i = curSec.index;
					pos = intersection;
				}
			}
	}
}