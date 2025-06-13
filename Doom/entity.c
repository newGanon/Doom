#include "entity.h"
#include "math.h"
#include "map.h"
#include "entityhandler.h"
#include "player.h"

void entity_calculate_relative_camera_position(EntityHandler* handler, Player* player) {
	for (u32 i = 0; i < handler->used; i++) {
		Entity* entity = handler->entities[i];
		v2 entityRelPos = world_pos_to_camera(entity->pos, player->pos, player->anglesin, player->anglecos);
		entity->relCamPos.y = entityRelPos.y;
		entity->relCamPos.x = entityRelPos.x;
	}
}

void tick_item(Entity* item) {
	const f32 speed = 150.0f * SECONDS_PER_UPDATE;
	const i32 ticks_total = 240;
	const i32 ticks_half = ticks_total / 2;
	const f32 max_z_offset = 0.8f;

	i32 curtick = (i32)item->animationtick;

	if (curtick > ticks_total / 2) {
		curtick = ticks_total - curtick;
	}
	f32 progress = (f32)curtick / ticks_half;
	f32 easeValue = ease_in_out_cubic(progress);
	item->z = (i32)item->z + (easeValue * max_z_offset);
	item->animationtick += speed;

	if (item->animationtick > ticks_total) {
		item->animationtick -= ticks_total;
	} 
}

void tick_enemy(Entity* enemy) {
	Player* player = enemy->target;
	v2 dpos = { 0.0f, 0.0f };
	f32 acceleration = 0.3f;
	f32 movespeed = enemy->speed * SECONDS_PER_UPDATE;
	bool chasing = false;

	if (fabsf(enemy->z - player->z) < 10.0f) {
		f32 dx = player->pos.x - enemy->pos.x;
		f32 dy = player->pos.y - enemy->pos.y;
		f32 len = dx * dx + dy * dy;
		len /= sqrtf(len);
		if (len < 15.0f) {
			acceleration = 0.4f;
			chasing = true;
			dpos = (v2) { dx / len, dy / len };
		}
	}

	if (!chasing) {
		i32 r = rand() % 100;
		switch (r) {
		case 0: dpos = (v2){ 0, 1.0f }; break;
		case 1: dpos = (v2){ 0.707f, 0.707f }; break;
		case 2: dpos = (v2){ 1.0f, 0 }; break;
		case 3: dpos = (v2){ 0.707f, -0.707f }; break;
		case 4: dpos = (v2){ 0, -1.0f }; break;
		case 5: dpos = (v2){ -0.707f, -0.707f }; break;
		case 6: dpos = (v2){ -1.0f, 0 }; break;
		case 7: dpos = (v2){ -0.707f, 0.707f }; break;
		default:break;
		}
	}

	enemy->velocity.x = enemy->velocity.x * (1 - acceleration) + dpos.x * acceleration * movespeed;
	enemy->velocity.y = enemy->velocity.y * (1 - acceleration) + dpos.y * acceleration * movespeed;

	entity_trymove(enemy, 1);
}

void tick_bullet(Entity* bullet) {
	bullet->velocity.x = bullet->dir.x * bullet->speed * SECONDS_PER_UPDATE;
	bullet->velocity.y = bullet->dir.y * bullet->speed * SECONDS_PER_UPDATE;

	bool hitwall = entity_trymove(bullet, 0);
	Sector* sec = map_get_sector(bullet->sector);
	if (hitwall || sec->zfloor > bullet->z || sec->zceil < bullet->z) {
		bullet->dirty = true;
	}
}

void entity_check_collisions(EntityHandler* handler, Player* p) {
	for (u32 i = 0; i < handler->used; i++) {
		Entity* e = handler->entities[i];
		if (e->dirty) continue;
		switch (e->type) {
		case Enemy: {
			break; 
		}
		case Projectile: {
			//friendly projectile
			if (!e->target){
				for (u32 j = 0; j < handler->used; j++) {
					Entity* tar = handler->entities[j];
					if (tar->dirty) continue;
					if (tar->type == Enemy && ((e->z - tar->z) < 10.0f)) {
						f32 dx = tar->pos.x - e->pos.x;
						f32 dy = tar->pos.y - e->pos.y;
						f32 r = sqrtf((dx * dx) + (dy * dy));
						if (r < tar->scale.x/2.0f) {
							tar->health -= e->damage;
							if (tar->health <= 0) {
								tar->dirty = true;
							}
							e->dirty = true;
						}
					}
				}
			}
			//enemy projectile
			else {

			}
			break;
		}
		case Item: {
			f32 r = sqrtf(e->relCamPos.x * e->relCamPos.x + e->relCamPos.y + e->relCamPos.y);
			//collect item
			if (r < 3.0f) {
				//TODO: give player item
				e->dirty = true;
			}
			break;
		}
		default: break;
		}
	}
}


bool entity_trymove(Entity* e, bool gravityactive) {
	Map* map = map_get_map();
	Sector curSec = *map_get_sector(e->sector);

	if (gravityactive) {
		//vertical collision detection
		const f32 gravity = -GRAVITY;

		if (e->airborne) {
			e->velocity.z += gravity;
			f32 dvel = e->velocity.z;
			//floor collision
			if (e->velocity.z < 0 && (e->z + dvel) < (curSec.zfloor + e->scale.y)) {
				e->velocity.z = 0;
				e->airborne = 0;
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
		if (get_line_intersection(pos, (v2) { pos.x + e->velocity.x, pos.y + e->velocity.y }, curwall->a, curwall->b, &intersection)) {
			f32 stepl = curwall->portal >= 0 ? map->sectors[curwall->portal].zfloor : 10e10f;
			f32 steph = curwall->portal >= 0 ? map->sectors[curwall->portal].zceil : -10e10f;
			if (e->type == Projectile && curwall->portal == -1) {
				v2 mappos = (v2){ intersection.x - curwall->a.x, intersection.y - curwall->a.y };
				v2 wallpos = (v2){ sqrtf(mappos.x * mappos.x + mappos.y * mappos.y), e->z };
				bool front = (POINTSIDE2D(pos.x, pos.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) < 0);
				if (map_spawn_decal(wallpos, curwall, (v2) { 2.0f, 2.0f }, 1, front)) {
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
			if (curwall->portal >= 0 && POINTSIDE2D(e->pos.x + e->velocity.x, e->pos.y + e->velocity.y, curwall->a.x, curwall->a.y, curwall->b.x, curwall->b.y) > 0) {
				curSec = map->sectors[curwall->portal];
				e->sector = curSec.id;
				i = curSec.index;
				if (e->type == Projectile) continue;
				if (e->z < e->scale.y + curSec.zfloor) e->z = e->scale.y + curSec.zfloor;
				else if (e->z > e->scale.y + curSec.zfloor) e->airborne = 1;
			}
		}
	}

	e->pos.x += e->velocity.x;
	e->pos.y += e->velocity.y;

	return hit;
}