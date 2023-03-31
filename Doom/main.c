#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"

typedef struct {
	v2 pos, dir, plane;
}Player;

struct {
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Renderer* renderer;
	i32 deltaTime;
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	f32 zBuffer[SCREEN_WIDTH];
	Texture textures[100];

	u8 quit;

	Player player;
} state;



void init();
void update();
void render();
void close();
void loadTextures(Texture* textures);
void rotatePlayer(f32 rot, Player* player);



int main(int argc, char* args[]) {

	init();

	i32 a, b = 0;
	SDL_Event e;

	while (!state.quit) {
		a = SDL_GetTicks();
		while (SDL_PollEvent(&e) != 0) {
			/*TODO: handle events*/
			switch (e.type) {
			case SDL_QUIT:
				close();
				break;
			case SDL_MOUSEMOTION:
				rotatePlayer(e.motion.xrel * ((f32)state.deltaTime / 1000.0f) * 0.1f, &state.player);
				break;
			}
		}
		state.deltaTime = a - b;
		if (state.deltaTime > SCREEN_TICKS_PER_FRAME) {
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
	//SDL_RenderClear(state.renderer);

	/* draw crosshair */
	/* drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 10, 10, RED); */
	fillRectangle(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT / 2 + 1, GREEN, &state.pixels);
	fillRectangle(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2 - 8, SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2 + 8, GREEN, &state.pixels);

	SDL_UpdateTexture(state.texture, NULL, &state.pixels, SCREEN_WIDTH * 4);
	SDL_RenderCopy(state.renderer, state.texture, NULL, NULL);
	SDL_RenderPresent(state.renderer);
}


void update() {

	const f32 movespeed = 2.0f * ((f32)state.deltaTime / 1000.0f), roationspeed = 2.0f * ((f32)state.deltaTime / 1000.0f);
	const u8* keyboardstate = SDL_GetKeyboardState(NULL);
	v2 moveVec = { 0, 0 };
	u8 move = 0;
	if (keyboardstate[SDL_SCANCODE_W]) {

	}
	if (keyboardstate[SDL_SCANCODE_S]) {

	}
	if (keyboardstate[SDL_SCANCODE_A]) {

	}
	if (keyboardstate[SDL_SCANCODE_D]) {

	}
}


void init() {
	ASSERT(!SDL_Init(SDL_INIT_VIDEO),
		"Error initializing SDL_Video %s\n",
		SDL_GetError());

	state.window = SDL_CreateWindow("RayCaster",
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
		SDL_RENDERER_PRESENTVSYNC);
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

	loadTextures(&state.textures);

	state.player.pos = (v2){ 3.0f, 2.0f };
	state.player.dir = v2Normalize((v2) { 1.0f, 0.0f });
	state.player.plane = (v2){ 0.0f, 1.0f };
}

void close() {

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
	return;
	SDL_Surface* bmpTex;
	char textureFileNames[1][50] = {"test.bmp"};
	i32 textureAmt = sizeof(textureFileNames) / sizeof(textureFileNames[0]);

	for (i32 i = 0; i < textureAmt; i++) {
		bmpTex = SDL_LoadBMP(textureFileNames[i]);
		ASSERT(bmpTex,
			"Error loading texture %s\n",
			SDL_GetError());
		state.textures[i] = (Texture){ (u32*)bmpTex->pixels, bmpTex->w, bmpTex->h };
	}
}

void rotatePlayer(f32 rot, Player* player) {
	player->dir = v2Rotate(player->dir, rot);
	player->plane = v2Rotate(player->plane, rot);
}