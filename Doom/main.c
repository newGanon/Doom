#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"
#include "entity.h"
#include "tick.h"
#include "map.h"
#include "player.h"
#include "tex.h"
#include "plat.h"
#include "entityhandler.h"

struct {
	// sdl
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Surface* surfaces[100];
	SDL_Renderer* renderer;
	// rendering
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	Palette lightmap;
	LightmapindexTexture index_textures[1000];
	// game logic and updates
	Map map;
	EntityHandler entityhandler;
	Player player;
	bool quit;
	bool KEYS[SDL_NUM_SCANCODES];
	f32 delta_time_acc;
} state;


void init();
void update();
void render();
void close();
void sdl_init();

// TODO: anchor texture at bottom of the wall and remember where oringinal anchor height is, to make texture stay when floor or ceiling moves
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
					state.player.anglecos = cosf(state.player.angle);
					state.player.anglesin = sinf(state.player.angle);
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
		state.delta_time_acc += a - b;
		b = a;
		if (state.delta_time_acc >= MS_PER_UPDATE) {
			while (state.delta_time_acc > MS_PER_UPDATE) {
				update();
				state.delta_time_acc -= MS_PER_UPDATE;
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
	draw_3d(&state.player, &state.entityhandler);
	draw_2d();

	u8* px = NULL;
	int pitch;
	if(!SDL_LockTexture(state.texture, NULL, &px, &pitch)) {
		for (u32 y = 0; y < SCREEN_HEIGHT; y++) {
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
	map_sort_walls(state.player.pos, state.player.anglesin, state.player.anglecos);
	player_tick(&state.player, &state.entityhandler, state.KEYS);
	entity_calculate_relative_camera_position(&state.entityhandler, &state.player);
	tickers_run();
	entity_check_collisions(&state.entityhandler, &state.player);
	entityhandler_removedirty(&state.entityhandler);
}

void init() {
	sdl_init();
	draw_init(state.pixels, &state.lightmap, state.index_textures);
	tex_init(state.surfaces, &state.lightmap, state.index_textures);
	map_init(&state.map);
	entityhandler_init(&state.entityhandler, 128);
	tickers_init();

	for (u32 i = 0; i < SDL_NUM_SCANCODES; i++) state.KEYS[i] = false;

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
		e1->dirty = false;
		ticker_add(&e1->tick);
		entity_add(&state.entityhandler, e1);
	}
	
	/*Entity* e2 = malloc(sizeof(Entity));
	if (e2) {
		e2->animationtick = 0;
		e2->airborne = 1;
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
		e2->dirty = false;
		e2->health = 10.0f;
		ticker_add(&e2->tick);
		entity_add(&state.entityhandler, e2);
	}*/

	// button to activate infinite up and down in sector 2 
	Decal* d = map_spawn_decal((v2) { 5.0f, 6.0f }, & state.map.walls[5], (v2) { 5.0f, 5.0f }, 1, true);
	d->tag = 1;
	d->tag_action = INFINITE_UP_DOWN;

	// button to activate raise staris in sector 9,10,11,12
	Decal* d2 = map_spawn_decal((v2) { 5.0f, 12.0f }, & state.map.walls[46], (v2) { 5.0f, 5.0f }, 1, true);
	d2->tag = 3;
	d2->tag_action = RAISE_STAIRS;

	//create_plat(1, INFINITE_UP_DOWN, true, 0, 0.0f);
	create_plat(2, INFINITE_UP_DOWN, false, 0, 0.0f);


	//state.player.pos = (v2){ 210.0f, 30.0f};
	//state.player.sector = 7;
	state.player.pos = (v2){ 22.0f, 15.0f};
	state.player.sector = 0;
	state.player.z = EYEHEIGHT + state.map.sectors[state.player.sector].zfloor;
	state.player.airborne = false;
	state.player.speed = PLAYERSPEED;

	state.player.angle = PI_2;
	state.player.anglecos = cosf(state.player.angle);
	state.player.anglesin = sinf(state.player.angle);
}

void close() {
	entityhandler_free(&state.entityhandler);
	tex_free(state.surfaces, state.index_textures);

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
