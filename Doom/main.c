#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"
#include "entity.h"
#include "ticker.h"
#include "tick.h"
#include "map.h"
#include "player.h"
#include "tex.h"
#include "plat.h"
#include <ctype.h>

struct {
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Surface* surfaces[100];
	SDL_Renderer* renderer;
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	Palette lightmap;
	LightmapindexTexture index_textures[1000];

	bool quit;
	EntityHandler entityhandler;
	Map map;
	Player player;
	bool KEYS[SDL_NUM_SCANCODES];
	f32 deltaTimeAcc;
} state;


void init();
void update();
void render();
void close();
void sdl_init();

// TODO: implement proper player and enitiy collision detection where player hitbox is a circle

int main(int argc, char* args[]) {
	init();
	
	i32 a, b = 0;
	SDL_Event e;

	//fps calc
	i32 last_render = SDL_GetTicks();

	while (!state.quit) {
		while (SDL_PollEvent(&e) != 0) {
			/*TODO: handle events*/
			switch (e.type) {
				case SDL_QUIT: {
					close();
					break; 
				}
				case SDL_MOUSEMOTION: {
					state.player.angle -= e.motion.xrel * PLAYERTOATIONSPEED;
					state.player.anglecos = cos(state.player.angle);
					state.player.anglesin = sin(state.player.angle);
					break; 
				}			
				case SDL_MOUSEBUTTONDOWN: {
					if (e.button.button == SDL_BUTTON_LEFT) {
						if (!state.player.shoot) {
							state.player.shoot = true;
						}
					}
					break; 
				}
				case SDL_KEYDOWN: {
					state.KEYS[e.key.keysym.scancode] = true;
					break;
				}
				case SDL_KEYUP: {
					state.KEYS[e.key.keysym.scancode] = false;
					break;
				}
			}
		}
		a = SDL_GetTicks();
		state.deltaTimeAcc += a - b;
		b = a;
		if (state.deltaTimeAcc >= MS_PER_UPDATE) {
			while (state.deltaTimeAcc > MS_PER_UPDATE) {
				update();
				state.deltaTimeAcc -= MS_PER_UPDATE;
			}
			render();
			//fps calc
			i32 cur_time = b;
			i32 render_diff = cur_time - last_render;
			last_render = cur_time;
			printf("FPS: %f\n", (1000.0f/ render_diff));
		}
	}
	close();
	return 0;

}
void render() {
	// clear pixel buffer
	memset(state.pixels, 0, sizeof(state.pixels));
	draw_3d(state.player, &state.entityhandler);

	/* draw crosshair */
	/* drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 10, 10, RED); */
	fill_rectangle(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT / 2 + 1, GREEN, &state.pixels);
	fill_rectangle(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2 - 8, SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2 + 8, GREEN, &state.pixels);

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
	sort_walls(&state.player);
	player_tick(&state.player, state.KEYS);
	sort_entities(&state.player);
	run_tickers();
	check_entity_collisions(&state.player);
}

void init() {
	sdl_init();
	draw_init(state.pixels, &state.lightmap, state.index_textures);
	init_tex(&state.surfaces, &state.lightmap, state.index_textures);
	load_level(&state.map);
	init_entityhandler(&state.entityhandler, 128);
	init_tickers();

	for (size_t i = 0; i < SDL_NUM_SCANCODES; i++) state.KEYS[i] = false;

	Entity* e1 = malloc(sizeof(Entity));
	if (e1) {
		e1->animationtick = 0;
		e1->pos = (v2){ 30.0f, 29.0f };
		e1->scale = (v2){ 4.0f, 4.0f };
		e1->sector = 0;
		e1->spriteAmt = 1;
		e1->spriteNum[0] = 1;
		e1->speed = 10;
		e1->tick.function = &tick_item;
		e1->type = Item;
		e1->z = 5.0f;
		add_ticker(&e1->tick);
		add_entity(e1);
	}
	
	Entity* e2 = malloc(sizeof(Entity));
	if (e2) {
		e2->animationtick = 0;
		e2->inAir = 1;
		e2->pos = (v2){ 25.0f, 35.0f };
		e2->scale = (v2){ 4.0f, 4.0f };
		e2->velocity = (v3){ 0, 0, 0 };
		e2->sector = 0;
		e2->spriteAmt = 1;
		e2->spriteNum[0] = 1;
		e2->speed = 10;
		e2->tick.function = &tick_enemy;
		e2->type = Enemy;
		e2->z = 2.0f;
		e2->target = &state.player;
		add_ticker(&e2->tick);
		add_entity(e2);
	}

	Decal* decal = malloc(sizeof(Decal));
	Decal* d = spawn_decal((v2) { 5.0f, 6.0f }, & state.map.walls[5], (v2) { 5.0f, 5.0f }, 1);
	d->tag = 1;

	state.map.sectors[6].tag = 1;
	//create_plat(1, INFINITE_UP_DOWN, true);

	state.map.sectors[5].tag = 2;
	create_plat(2, INFINITE_UP_DOWN, false);


	state.player.pos = (v2){ 20.0f, 20.0f};
	state.player.sector = 0;
	state.player.z = EYEHEIGHT + state.map.sectors[state.player.sector].zfloor;
	state.player.in_air = false;
	state.player.speed = 20.0f;

	state.player.angle = PI_2;
	state.player.anglecos = cos(state.player.angle);
	state.player.anglesin = sin(state.player.angle);
}

void close() {
	free_entityhandler();

	for (size_t i = 0; i < 100; i++){
		SDL_Surface* surface = state.surfaces[i];
		if (surface == NULL) break;
		SDL_FreeSurface(surface);
	}

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
