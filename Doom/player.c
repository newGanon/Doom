#include <SDL.h>
#include "player.h"
#include "math.h"
#include "map.h"
#include "entity.h"
#include "plat.h"

void calc_playervelocity(Player* p, bool* KEYS);
void check_shoot(Player* p, EntityHandler* handler);
void p_interact(Player* p);

void player_tick(Player* p, EntityHandler* handler, bool* KEYS) {
	//const u8* keyboardstate = SDL_GetKeyboardState(NULL);

	// check for player pressing interaction key
	if (KEYS[SDL_SCANCODE_E]) {
		p_interact(p);
		KEYS[SDL_SCANCODE_E] = false;
	}

	p->sneak = KEYS[SDL_SCANCODE_LSHIFT] ? true : false;
	p->speed = p->sneak ? PLAYERSNEAKSPEED : PLAYERSPEED;

	calc_playervelocity(p, KEYS);
	trymove_player(p);
	check_shoot(p, handler);

	// reset player pos
	if (KEYS[SDL_SCANCODE_R] || p->dead) {
		p->pos = (v2){ 20.0f, 20.0f };
		p->z = EYEHEIGHT;
		p->sector = 0;
		KEYS[SDL_SCANCODE_R] = false;
		p->dead = false;
	}
}


void calc_playervelocity(Player* p, bool* KEYS) {
	const f32 movespeed = p->speed * SECONDS_PER_UPDATE;
	//const u8* keyboardstate = SDL_GetKeyboardState(NULL);

	//vertical velocity calc
	if (KEYS[SDL_SCANCODE_SPACE] && !p->in_air) {
		p->in_air = true;
		p->velocity.z = 30.0f;
	}

	//horizontal velocity calc
	v2 dpos = { 0, 0 };

	if (KEYS[SDL_SCANCODE_W]) { dpos.x += p->anglecos; dpos.y += p->anglesin; }
	if (KEYS[SDL_SCANCODE_S]) { dpos.x -= p->anglecos; dpos.y -= p->anglesin; }
	if (KEYS[SDL_SCANCODE_A]) { dpos.x -= p->anglesin; dpos.y += p->anglecos; }
	if (KEYS[SDL_SCANCODE_D]) { dpos.x += p->anglesin; dpos.y -= p->anglecos;}

	u8 moved = KEYS[SDL_SCANCODE_W] || KEYS[SDL_SCANCODE_S] || KEYS[SDL_SCANCODE_A] || KEYS[SDL_SCANCODE_D];

	f32 acceleration = moved ? 0.4 : 0.3;

	p->velocity.x = p->velocity.x * (1 - acceleration) + dpos.x * acceleration * movespeed;
	p->velocity.y = p->velocity.y * (1 - acceleration) + dpos.y * acceleration * movespeed;

	//p->velocity.x = dpos.x;
	//p->velocity.y = dpos.y;
}


void check_shoot(Player* p, EntityHandler* handler) {
	if (!p->shoot) return;
	p->shoot = false;
	i32 weapon = 0;

	switch (weapon) {
		case 0: {
			Entity* bullet = malloc(sizeof(Entity));
			if (bullet) {
				bullet->tick.function = &tick_bullet;
				bullet->pos = (v2){ p->pos.x, p->pos.y };
				bullet->speed = 80.0f;
				bullet->z = p->z;
				bullet->scale = (v2){ 2.0f, 2.0f };
				bullet->spriteAmt = 1;
				bullet->spriteNum[0] = 1;
				bullet->type = Projectile;
				bullet->velocity = (v3){ p->anglecos, p->anglesin, 0 };
				bullet->sector = p->sector;
				bullet->target = NULL;
				bullet->dirty = false;
				bullet->damage = 2.0f;
				ticker_add(&bullet->tick);
				entity_add(handler, bullet);
			}
			break;
		}
		case 1: {
			RaycastResult res = raycast(get_sector(p->sector), p->pos, (v2) { p->pos.x + p->anglecos * 1000.0f, p->pos.y + p->anglesin * 1000.0f }, p->z);
			if (res.hit) {
				v2 wallpos = (v2){ sqrt(res.wall_pos.x * res.wall_pos.x + res.wall_pos.y * res.wall_pos.y), p->z };
				spawn_decal(wallpos, res.wall, (v2) { 2.0f, 2.0f }, 1);
			}
			break;
		}
		default: break;
	}
}

void p_interact(Player* p) {
	Sector* cursec = get_sector(p->sector);
	RaycastResult res = raycast(cursec, p->pos, (v2) { p->pos. x + p->anglecos * 1000.0f, p->pos.y + p->anglesin * 1000.0f }, p->z);
	if (res.hit) {
		if (res.distance > (15.0f)) return;
		for (Decal* d = res.wall->decalhead; d != NULL; d = d->next) {
			get_relative_decal_wall_height(d, res.wall, cursec->zfloor);
			v2 rel_wallpos = (v2){ sqrt(res.wall_pos.x * res.wall_pos.x + res.wall_pos.y * res.wall_pos.y), p->z };
			if (rel_wallpos.x > d->wallpos.x && rel_wallpos.x < (d->wallpos.x + d->size.x) && rel_wallpos.y > d->wallpos.y && rel_wallpos.y < (d->wallpos.y + d->size.y)) {
				create_plat(d->tag, INFINITE_UP_DOWN, true);
			}
		}
	}
}