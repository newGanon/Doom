#include <SDL.h>
#include "player.h"
#include "math.h"
#include "map.h"
#include "entity.h"

void calc_playervelocity(Player* p);
void check_shoot(Player* p);

void player_tick(Player* p) {
	const u8* keyboardstate = SDL_GetKeyboardState(NULL);

	//move player
	calc_playervelocity(p);
	trymove_player(p);

	check_shoot(p);
	
	//reset player pos
	if (keyboardstate[SDL_SCANCODE_R]) {
		p->pos = (v2){ 15.0f, 15.0f};
		p->z = EYEHEIGHT;
		p->sector = 1; 
	}
}


void calc_playervelocity(Player* p) {
	const f32 movespeed = p->speed * FRAMETICKS;
	const u8* keyboardstate = SDL_GetKeyboardState(NULL);

	//vertical velocity calc
	if (keyboardstate[SDL_SCANCODE_SPACE] && !p->inAir) {
		p->inAir = 1;
		p->velocity.z = 25.0f;
	}

	//horizontal velocity calc
	v2 dpos = { 0, 0 };

	if (keyboardstate[SDL_SCANCODE_W]) { dpos.x += p->anglecos; dpos.y += p->anglesin; }
	if (keyboardstate[SDL_SCANCODE_S]) { dpos.x -= p->anglecos; dpos.y -= p->anglesin; }
	if (keyboardstate[SDL_SCANCODE_A]) { dpos.x -= p->anglesin; dpos.y += p->anglecos; }
	if (keyboardstate[SDL_SCANCODE_D]) { dpos.x += p->anglesin; dpos.y -= p->anglecos;}

	u8 moved = keyboardstate[SDL_SCANCODE_W] || keyboardstate[SDL_SCANCODE_S] || keyboardstate[SDL_SCANCODE_A] || keyboardstate[SDL_SCANCODE_D];

	f32 acceleration = moved ? 0.4 : 0.3;

	//p->velocity.x = p->velocity.x * (1 - acceleration) + dpos.x * acceleration * movespeed;
	//p->velocity.y = p->velocity.y * (1 - acceleration) + dpos.y * acceleration * movespeed;

	p->velocity.x = dpos.x;
	p->velocity.y = dpos.y;
}


void check_shoot(Player* p) {
	if (!p->shoot) return;
	p->shoot = 0;
	i32 weapon = 1;

	switch (weapon) {
		case 0: {
			Entity* bullet = malloc(sizeof(Entity));
			if (bullet) {
				bullet->tick.function = &tick_bullet;
				bullet->pos = (v2){ p->pos.x, p->pos.y };
				bullet->speed = 80.0f;
				bullet->z = p->z;
				bullet->scale = (v2){ 1.0f, 1.0f };
				bullet->spriteAmt = 1;
				bullet->spriteNum[0] = 1;
				bullet->type = Projectile;
				bullet->velocity = (v3){ p->anglecos, p->anglesin, 0 };
				bullet->sector = p->sector;
				bullet->target = NULL;
				add_ticker(&bullet->tick);
				addEntity(bullet);
			}
		}
		case 1: {
			check_hitscan_collsion(p);
			break;
		}
		default: break;
	}
}