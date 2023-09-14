#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"
#include "entity.h"
#include "ticker.h"
#include "tick.h"
#include "map.h"
#include "player.h"
#include "tex.h";
#include <ctype.h>

struct {
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Surface* surfaces[100];
	SDL_Renderer* renderer;
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	Texture textures[1000];

	EntityHandler entityhandler;

	Map map;

	u8 quit;

	Player player;
} state;


void init();
void update();
void render();
void close();
void sdl_init();


int main(int argc, char* args[]) {

	init();

	i32 a, b = 0;
	SDL_Event e;

	while (!state.quit) {
		a = SDL_GetTicks();
		while (SDL_PollEvent(&e) != 0) {
			/*TODO: handle events*/
			switch (e.type) {
			case SDL_QUIT: {
				close();
				break; }
			case SDL_MOUSEMOTION: {
				state.player.angle -= e.motion.xrel * PLAYERTOATIONSPEED;
				state.player.anglecos = cos(state.player.angle);
				state.player.anglesin = sin(state.player.angle); }
								break;
			case SDL_MOUSEBUTTONDOWN: {
				if (e.button.button == SDL_BUTTON_LEFT) {
					if (!state.player.shoot) {
						state.player.shoot = 1;
					}
				}
				break; }
			}
		}
		deltaTime = a - b;
		if (deltaTime > SCREEN_TICKS_PER_FRAME) {
			//printf("%i\n", 1000/(a - b));
			b = a;
			update();
			render();
		}
	}
	close();
	return 0;

}
void render() {

	memset(state.pixels, 0, sizeof(state.pixels));

	draw3D(state.player, &state.textures, &state.entityhandler);

	/* draw crosshair */
	/* drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 10, 10, RED); */
	fillRectangle(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT / 2 + 1, GREEN, &state.pixels);
	fillRectangle(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2 - 8, SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2 + 8, GREEN, &state.pixels);

	u8* px;
	int pitch;
	SDL_LockTexture(state.texture, NULL, &px, &pitch);
	{
		for (usize y = 0; y < SCREEN_HEIGHT; y++) {
			memcpy(&px[y * pitch], &state.pixels[y * SCREEN_WIDTH], SCREEN_WIDTH * 4);
		}
	}
	SDL_UnlockTexture(state.texture);

	SDL_SetRenderTarget(state.renderer, NULL);
	SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 0xFF);
	SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);

	SDL_RenderClear(state.renderer);
	SDL_RenderCopyEx(state.renderer, state.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);

	SDL_RenderPresent(state.renderer);
}


void update() {

	sortWalls(&state.player);

	player_tick(&state.player);

	sortEntities(&state.player);

	run_tickers();

	check_entitycollisions(&state.player);
}

void init() {

	sdl_init();

	drawInit(state.pixels);

	init_tex(&state.textures);
	loadTextures(&state.surfaces);

	loadLevel(&state.map);

	initEntityHandler(&state.entityhandler, 128);

	init_tickers();

	Entity* e = malloc(sizeof(Entity));
	if (e) {
		e->animationtick = 0;
		e->pos = (v2){ 22.0f, 22.0f };
		e->scale = (v2){ 3.0f, 3.0f };
		e->sector = 1;
		e->spriteAmt = 1;
		e->spriteNum[0] = 2;
		e->speed = 10;
		e->tick.function = &tick_item;
		e->type = Item;
		e->z = 2.5f;
		add_ticker(&e->tick);
		addEntity(e);
	}

	Entity* e1 = malloc(sizeof(Entity));
	if (e1) {
		e1->animationtick = 0;
		e1->pos = (v2){ 30.0f, 29.0f };
		e1->scale = (v2){ 3.0f, 3.0f };
		e1->sector = 1;
		e1->spriteAmt = 1;
		e1->spriteNum[0] = 2;
		e1->speed = 10;
		e1->tick.function = &tick_item;
		e1->type = Item;
		e1->z = 2.5f;
		add_ticker(&e1->tick);
		addEntity(e1);
	}


	/*Entity* e2 = malloc(sizeof(Entity));
	if (e2) {
		e2->animationtick = 0;
		e2->inAir = 1;
		e2->pos = (v2){ 25.0f, 35.0f };
		e2->scale = (v2){ 4.0f, 4.0f };
		e2->velocity = (v3){ 0, 0, 0 };
		e2->sector = 1;
		e2->spriteAmt = 1;
		e2->spriteNum[0] = 1;
		e2->speed = 10;
		e2->tick.function = &tick_enemy;
		e2->type = Enemy;
		e2->z = 5.0f;
		e2->target = &state.player;
		add_ticker(&e2->tick);
		addEntity(e2);
	}*/

	state.player.pos = (v2){ 20.0f, 20.0f};
	state.player.sector = 1;
	state.player.z = EYEHEIGHT + state.map.sectors[state.player.sector - 1].zfloor;
	state.player.inAir = 0;
	state.player.speed = 20.0f;

	state.player.angle = PI_2;
	state.player.anglecos = cos(state.player.angle);
	state.player.anglesin = sin(state.player.angle);

	/*Decal* decal = malloc(sizeof(Decal));
	if (decal) {
		decal->tex = &state.textures[1];
		decal->offset = (v2i){ 500, 500 };
		decal->next = NULL;
		decal->prev = NULL;
		decal->scale = 0.5f;
		decal->scaledsize = (v2i){ decal->tex->width * decal->scale, decal->tex->height * decal->scale};

		state.map.walls[0].decalhead = decal;
	}*/
	/*
	Decal* decal2 = malloc(sizeof(Decal));
	if (decal2) {
		decal2->tex = &state.textures[1];
		decal2->offset = (v2i){ 200, 200 };
		decal2->next = NULL;
		decal2->prev = NULL;
		decal2->scale = 2.0f;
		decal2->scaledsize = (v2i){ decal2->tex->width * decal2->scale, decal2->tex->height * decal2->scale };

		state.map.walls[0].decalhead->next = decal2;
		decal2->prev = state.map.walls[0].decalhead;
	}*/
}

void close() {
	//TODO free entityman and surfaces
	SDL_DestroyWindow(state.window);
	state.window = NULL;

	SDL_DestroyTexture(state.texture);
	state.texture = NULL;

	SDL_DestroyRenderer(state.renderer);
	state.renderer = NULL;

	SDL_Quit();
	exit(1);
}

void sdl_init() {
	ASSERT(!SDL_Init(SDL_INIT_VIDEO),
		"Error initializing SDL_Video %s\n",
		SDL_GetError());

	state.window = SDL_CreateWindow("Doom",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN);
	ASSERT(state.window,
		"Error creating SDL_Window %s\n",
		SDL_GetError());

	state.renderer = SDL_CreateRenderer(state.window,
		-1,
		SDL_RENDERER_ACCELERATED);
	ASSERT(state.renderer,
		"Error creating SDL_Renderer %s\n",
		SDL_GetError());

	state.texture = SDL_CreateTexture(state.renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH,
		SCREEN_HEIGHT);
	ASSERT(state.texture,
		"Error creating SDL_Texture %s\n",
		SDL_GetError());

	SDL_SetRelativeMouseMode(SDL_TRUE);
}
