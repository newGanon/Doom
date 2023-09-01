#include "entity.h"
#include "math.h"


void initEntityHandler(EntityHandler* h, u32 initialSize) {
	h->entities = malloc(initialSize * sizeof(Entity));
	h->used = 0;
	h->size = initialSize;
}

void addEntity(EntityHandler* h, Entity entity) {
	if (h->used == h->size) {
		h->size *= 2;
		Entity* tmp = (Entity*)realloc(h->entities, h->size * sizeof(Entity));
		ASSERT(!(tmp == NULL), "error while using realloc");
		h->entities = tmp;
	}
	h->entities[h->used++] = entity;
}

void freeEntities(EntityHandler* h) {
	free(h->entities);
	h->entities = NULL;
	h->used = h->size = 0;
}

void removeEntityAt(EntityHandler* h, i32 index) {
	h->entities[index] = h->entities[h->used - 1];
	h->used--;
}

void removeEntity(EntityHandler* h, Entity* e) {
	for (u32 i = 0; i < h->used; i++) {
		if (&h->entities[i] == e) {
			h->entities[i] = h->entities[h->used - 1];
			h->used--;
		}
	}
}

//not used
void sortEntities(EntityHandler* h, Player* player) {

	calcAllRelCamPos(h, player);
	Entity temp;
	if (h->used == 0) return;
	//relCamPos.y is the distance from the camera
	for (u32 i = 0; i < h->used - 1; i++)
	{
		if (h->entities[i].relCamPos.y < h->entities[i + 1].relCamPos.y) {
			temp = h->entities[i];
			h->entities[i] = h->entities[i + 1];
			h->entities[i + 1] = temp;
		}
	}
}

void calcAllRelCamPos(EntityHandler* h, Player* player) {
	for (u32 i = 0; i < h->used; i++) {
		v2 entityRelPos = world_pos_to_camera(h->entities[i].pos, *player);
		h->entities[i].relCamPos.y = entityRelPos.y;
		h->entities[i].relCamPos.x = entityRelPos.x;
	}
}


void tick_item(Entity* item) {
	i32 totalanimationticks = 160;
	i32 curtick = item->animationtick;
	if (curtick > totalanimationticks / 2) curtick = totalanimationticks - curtick;
	f32 easeValue = easeInOutCubic((f32)curtick / (totalanimationticks / 2));
	item->vMove = (i32)item->vMove + (easeValue * 0.99);
	item->animationtick += 1;
	item->animationtick = item->animationtick % (totalanimationticks);
}

void tick_enemy(Entity* enemy) {
	Player* player = enemy->target;
	if (abs(enemy->z + enemy->vMove - player->pos.z) < 10.0f) {
		f32 dx = player->pos.x - enemy->pos.x;
		f32 dy = player->pos.y - enemy->pos.y;
		f32 len = dx * dx + dy * dy;
		len /= sqrt(len);
		if (len < 10.0f) {
			enemy->velocity.x = dx / len * 0.04f;
			enemy->velocity.y = dy / len * 0.04f;
		}
	}

	enemy->pos.x += enemy->velocity.x;
	enemy->pos.y += enemy->velocity.y;
	enemy->z += enemy->velocity.z;

	enemy->velocity = (v3){ 0,0,0 };
}