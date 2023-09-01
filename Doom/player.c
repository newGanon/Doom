#include <SDL.h>
#include "player.h"
#include "math.h"
#include "map.h"


void player_tick(Player* p) {
	const f32 movespeed = p->speed * ((f32)deltaTime / 1000.0f);
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
	if (keyboardstate[SDL_SCANCODE_D]) { dpos.x += p->anglesin; dpos.y -= p->anglecos; }

	u8 moved = keyboardstate[SDL_SCANCODE_W] || keyboardstate[SDL_SCANCODE_S] || keyboardstate[SDL_SCANCODE_A] || keyboardstate[SDL_SCANCODE_D];

	f32 acceleration = moved ? 0.4 : 0.3;

	p->velocity.x = p->velocity.x * (1 - acceleration) + dpos.x * acceleration * movespeed;
	p->velocity.y = p->velocity.y * (1 - acceleration) + dpos.y * acceleration * movespeed;

	trymove_player(p);
	
	//reset player pos
	if (keyboardstate[SDL_SCANCODE_R]) {
		p->pos = (v2){ 15.0f, 15.0f};
		p->z = EYEHEIGHT;
		p->sector = 1; 
	}
}