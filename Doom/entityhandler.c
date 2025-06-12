#include "entityhandler.h"
#include "entity.h"


void entityhandler_init(EntityHandler* handler, u32 initialsize) {
	handler->entities = malloc(initialsize * sizeof(Entity*));
	handler->used = 0;
	handler->size = initialsize;
}


void entity_add(EntityHandler* handler, Entity* entity) {
	if (handler->used == handler->size) {
		handler->size = handler->size << 1;
		Entity* hand_p = (Entity*)realloc(handler->entities, handler->size * sizeof(Entity*));
		ASSERT(!(hand_p == NULL), "error while using realloc");
		handler->entities = hand_p;
	}
	handler->entities[handler->used++] = entity;
}


void entityhandler_free(EntityHandler* handler) {
	free(handler->entities);
	handler->entities = NULL;
	handler->used = handler->size = 0;
	*handler = (EntityHandler){ 0 };
}


void static entity_remove(EntityHandler* handler, u32 entity_id) {
	Entity* entity = handler->entities[entity_id];
	if (!entity) return;
	if (entity->tick.function != (actionf)(-1)) ticker_remove(&entity->tick); 
	free(entity);
	handler->entities[entity_id] = handler->entities[--handler->used];
}


void entityhandler_removedirty(EntityHandler* handler) {
	for (u32 i = 0; i < handler->used; i++) {
		if (handler->entities[i]->dirty) {
			entity_remove(handler, i);
			i--;
		}
	}
}