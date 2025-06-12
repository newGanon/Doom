#include "entity.h"
#include "math.h"
#include "map.h"

void calc_all_rel_cam_pos(EntityHandler* handler, Player* player) {
	for (u32 i = 0; i < handler->used; i++) {
		Entity* entity = handler->entities[i];
		v2 entityRelPos = world_pos_to_camera(entity->pos, *player);
		entity->relCamPos.y = entityRelPos.y;
		entity->relCamPos.x = entityRelPos.x;
	}
}

void tick_item(Entity* item) {
	f32 speed = 150.0f * SECONDS_PER_UPDATE;
	i32 totalanimationticks = 240;
	i32 curtick = item->animationtick;
	if (curtick > totalanimationticks / 2) curtick = totalanimationticks - curtick;
	f32 easeValue = ease_in_out_cubic((f32)curtick / (totalanimationticks / 2));
	item->z = (i32)item->z + (easeValue * 0.99);
	item->animationtick += speed;
	if (item->animationtick > totalanimationticks) item->animationtick -= totalanimationticks;
}

void tick_enemy(Entity* enemy) {
	Player* player = enemy->target;
	v2 dpos = { 0, 0 };
	f32 acceleration = 0.3;
	f32 movespeed = enemy->speed * SECONDS_PER_UPDATE;
	bool chasing = false;
	

	if (abs(enemy->z - player->z) < 10.0f) {
		f32 dx = player->pos.x - enemy->pos.x;
		f32 dy = player->pos.y - enemy->pos.y;
		f32 len = dx * dx + dy * dy;
		len /= sqrt(len);
		if (len < 15.0f) {
			acceleration = 0.4;
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

	trymove_entity(enemy, 1);
}

void tick_bullet(Entity* bullet) {
	bullet->velocity.x = bullet->dir.x * bullet->speed * SECONDS_PER_UPDATE;
	bullet->velocity.y = bullet->dir.y * bullet->speed * SECONDS_PER_UPDATE;

	bool hitwall = trymove_entity(bullet, 0);
	Sector* sec = get_sector(bullet->sector);
	if (hitwall || sec->zfloor > bullet->z || sec->zceil < bullet->z) {
		bullet->dirty = true;
	}
}

void check_entity_collisions(EntityHandler* handler, Player* p) {
	for (i32 i = 0; i < handler->used; i++) {
		Entity* e = handler->entities[i];
		if (e->dirty) continue;
		switch (e->type) {
		case Enemy: {
			break; 
		}
		case Projectile: {
			//friendly projectile
			if (!e->target){
				for (i32 j = 0; j < handler->used; j++) {
					Entity* tar = handler->entities[j];
					if (tar->dirty) continue;
					if (tar->type == Enemy && ((e->z - tar->z) < 10.0f)) {
						f32 dx = tar->pos.x - e->pos.x;
						f32 dy = tar->pos.y - e->pos.y;
						f32 r = sqrt((dx * dx) + (dy * dy));
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
			f32 r = sqrt(e->relCamPos.x * e->relCamPos.x + e->relCamPos.y + e->relCamPos.y);
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