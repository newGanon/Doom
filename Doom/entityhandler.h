#pragma once
#include "util.h"
#include "entity.h"

typedef struct EntityHandler_s {
	Entity** entities;
	u32 used;
	u32 size;
}EntityHandler;

void entityhandler_init(EntityHandler* handler, u32 initialsize);
void entity_add(EntityHandler* handler, Entity* entity);
void entityhandler_free(EntityHandler* handler);
void entityhandler_removedirty(EntityHandler* handler);