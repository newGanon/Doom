#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"
#include "entity.h"
#include "ticker.h"
#include "tick.h"
#include "map.h"
#include "player.h"
#include <ctype.h>

struct {
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Surface* surfaces[100];
	SDL_Renderer* renderer;
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	Texture textures[100];

	EntityHandler entityhandler;

	Map map;

	u8 quit;

	Player player;
} state;


void init();
void update();
void render();
void close();
void loadTextures(Texture* textures);


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
			printf("%i\n", 1000/(a - b));
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

	calcAllRelCamPos(&state.entityhandler, &state.player);

	player_tick(&state.player);

	run_tickers();
}

void init() {
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

	drawInit(state.pixels);

	loadTextures(&state.textures);

	loadLevel(&state.map);

	initEntityHandler(&state.entityhandler, 128);

	init_tickers();


	Entity e = (Entity){
		.tick.function = &tick_item,
		.pos = {22.0f, 22.0f},
		.vMove = 2.5f,
		.speed = 0,
		.damage = 0,
		.scale = { 3.0f, 3.0f },
		.spriteAmt = 1,
		.spriteNum = { 2 },
		.type = Item,
		.animationtick = 0,
	};
	addEntity(&state.entityhandler,e);

	add_ticker(&state.entityhandler.entities[0].tick);


	Entity e1 = (Entity){
		//.tick.function = (actionf)(-1),
		.tick.function = &tick_enemy,
		.pos = {18.0f, 18.0f},
		.vMove = 6.0f,
		.speed = 0,
		.damage = 0,
		.scale = { 7.5f, 7.5f },
		.spriteAmt = 1,
		.spriteNum = { 1 },
		.type = Enemy,
		.animationtick = 0,
		.target = &state.player
	};

	addEntity(&state.entityhandler, e1);

	add_ticker(&state.entityhandler.entities[1].tick);

	state.player.pos = (v2){ 20.0f, 20.0f};
	state.player.sector = 1;
	state.player.z = EYEHEIGHT + state.map.sectors[state.player.sector - 1].zfloor;
	state.player.inAir = 0;
	state.player.speed = 20.0f;

	state.player.angle = PI_2;
	state.player.anglecos = cos(state.player.angle);
	state.player.anglesin = sin(state.player.angle);
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

void loadTextures(Texture* textures) {
	SDL_Surface* bmpTex;
	char textureFileNames[3][50] = {"test2.bmp", "spritetest2.bmp", "ammo.bmp"};
	i32 textureAmt = sizeof(textureFileNames) / sizeof(textureFileNames[0]);

	for (i32 i = 0; i < textureAmt; i++) {
		bmpTex = SDL_LoadBMP(textureFileNames[i]);
		ASSERT(bmpTex,
			"Error loading texture %s\n",
			SDL_GetError());
		state.textures[i] = (Texture){ (u32*)bmpTex->pixels, bmpTex->w, bmpTex->h };
		state.surfaces[i] = bmpTex;
	}
}


