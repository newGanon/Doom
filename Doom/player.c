#include <SDL.h>
#include "player.h"
#include "math.h"
#include "map.h"
#include "entity.h"
#include "plat.h"

void calc_playervelocity(Player* p, bool* KEYS);
void player_check_shoot(Player* p, EntityHandler* handler);
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
	player_trymove(p);
	player_check_shoot(p, handler);

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
	if (KEYS[SDL_SCANCODE_SPACE] && !p->airborne) {
		p->airborne = true;
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


void player_check_shoot(Player* p, EntityHandler* handler) {
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
				bullet->dir = (v2){ p->anglecos, p->anglesin };
				bullet->velocity = (v3){ 0 };
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



void player_trymove(Player* p) {
	//vertical collision detection
	const f32 gravity = -GRAVITY * SECONDS_PER_UPDATE;
	Map* map = get_map();
	Sector* cur_sec = get_sector(p->sector);
	f32 eyeheight = p->sneak ? SNEAKHEIGHT : EYEHEIGHT;

	// player above ground
	if (cur_sec->zfloor < p->z - eyeheight && !p->airborne) {
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


	if (p->airborne) {
		p->velocity.z += gravity;
		f32 dvel = p->velocity.z * SECONDS_PER_UPDATE;
		//floor collision
		if (p->velocity.z < 0 && (p->z + dvel) < (cur_sec->zfloor + eyeheight)) {
			p->velocity.z = 0;
			p->airborne = false;
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
	bool in_air_old = p->airborne;
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
						(p->sneak && !p->airborne && (cur_sec->zfloor - stepl) > 0.0f)) {
						//if player hit a corner set velocity to 0
						if (wallind != -1) {
							p->velocity.x = 0;
							p->velocity.y = 0;
							cur_sec = sec_old;
							p->sector = sec_old->id;
							p->airborne = in_air_old;
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
			else if (p->z > eyeheight + cur_sec->zfloor) p->airborne = true;
		}
	}

	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;
}