#include "entity.h"
#include "math.h"
#include "map.h"

EntityHandler* h;

void initEntityHandler(EntityHandler* h1, u32 initialSize) {
	h = h1;
	h->entities = malloc(initialSize * sizeof(Entity*));
	h->used = 0;
	h->size = initialSize;
}

void addEntity(Entity* entity) {
	if (h->used == h->size) {
		h->size *= 2;
		Entity* tmp = (Entity*)realloc(h->entities, h->size * sizeof(Entity));
		ASSERT(!(tmp == NULL), "error while using realloc");
		h->entities = tmp;
	}
	h->entities[h->used++] = entity;
}

void free_entitiehandler() {
	for (u32 i = 0; i < h->used; i++) { free(h->entities[i]); }
	free(h->entities);
	h->entities = NULL;
	h->used = h->size = 0;
}

void removeEntity(Entity* e) {
	for (u32 i = 0; i < h->used; i++) {
		if (h->entities[i] == e) {
			free(e);
			h->entities[i] = h->entities[h->used - 1];
			h->used--;
			break;
		}
	}
}

void sortEntities(Player* player) {

	if (h->used == 0) return;
	calcAllRelCamPos(player);

	//relCamPos.y is the distance from the camera
	for (u32 i = 0; i < h->used - 1; i++)
	{
		if (h->entities[i]->relCamPos.y < h->entities[i + 1]->relCamPos.y) {

			Entity* temp = h->entities[i];
			h->entities[i] = h->entities[i + 1];
			h->entities[i + 1] = temp;
		}
	}
}

void calcAllRelCamPos(Player* player) {
	for (u32 i = 0; i < h->used; i++) {
		v2 entityRelPos = world_pos_to_camera(h->entities[i]->pos, *player);
		h->entities[i]->relCamPos.y = entityRelPos.y;
		h->entities[i]->relCamPos.x = entityRelPos.x;
	}
}


void tick_item(Entity* item) {
	f32 speed = 250.0f * FRAMETICKS;
	i32 totalanimationticks = 240;
	i32 curtick = item->animationtick;
	if (curtick > totalanimationticks / 2) curtick = totalanimationticks - curtick;
	f32 easeValue = easeInOutCubic((f32)curtick / (totalanimationticks / 2));
	item->z = (i32)item->z + (easeValue * 0.99);
	item->animationtick += speed;
	if (item->animationtick > totalanimationticks) item->animationtick -= totalanimationticks;
}

void tick_enemy(Entity* enemy) {
	Player* player = enemy->target;
	v2 dpos = { 0, 0 };
	f32 acceleration = 0.3;
	f32 movespeed = enemy->speed * FRAMETICKS;
	u8 chasing = 0;

	if (abs(enemy->z - player->z) < 10.0f) {
		f32 dx = player->pos.x - enemy->pos.x;
		f32 dy = player->pos.y - enemy->pos.y;
		f32 len = dx * dx + dy * dy;
		len /= sqrt(len);
		if (len < 15.0f) {
			acceleration = 0.4;
			chasing = 1;
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
	f32 movespeed = bullet->speed * FRAMETICKS;
	v3 oldvel = bullet->velocity;
	
	bullet->velocity.x *= movespeed;
	bullet->velocity.y *= movespeed;


	u8 hitwall = trymove_entity(bullet, 0);
	if (hitwall) {
		remove_ticker(&bullet->tick);
		removeEntity(bullet);
	}
	else {
		bullet->velocity = oldvel;
	}
}

void check_entitycollisions(Player* p) {
	for (i32 i = 0; i < h->used; i++) {
		Entity* e = h->entities[i];
		switch (e->type) {
		case Enemy: {
			break; 
		}
		case Projectile: {
			//friendly projectile
			if (!e->target){
				for (i32 j = 0; j < h->used; j++) {
					Entity* tar = h->entities[j];
					if (tar->type == Enemy) {
						if ((e->z - tar->z) < 10.0f) {
							f32 dx = tar->pos.x - e->pos.x;
							f32 dy = tar->pos.y - e->pos.y;
							f32 r = sqrt((dx * dx) + (dy * dy));
							if (r < tar->scale.x/2.0f) {
								tar->health -= e->damage;
								//TODO: only for testing, proper death checking
								free_and_remove_entity(e);
								free_and_remove_entity(tar);
							}
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
				//TODO: give player something
				free_and_remove_entity(e);
			}
			break;
		}
		default: break;
		}
	}
}

//frees entity and removes it from ticklist and entityhandler
void free_and_remove_entity(Entity* e) {
	if (e->tick.function != (actionf)(-1)) { remove_ticker(&e->tick); }
	removeEntity(e);
}