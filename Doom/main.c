#include <SDL.h>
#include "util.h"
#include "draw.h"
#include "math.h"

#include <ctype.h>

struct {
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Renderer* renderer;
	i32 deltaTime;
	u32 pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
	Texture textures[100];

	Map map;

	u8 quit;

	Player player;
} state;


void init();
void update();
void render();
void close();
void loadTextures(Texture* textures);
void loadLevel();


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
			case SDL_MOUSEMOTION: {
				f32 rotaionspeed = 0.1f * ((f32)state.deltaTime / 1000.0f);
				state.player.angle -= e.motion.xrel * rotaionspeed; 
				state.player.anglecos = cos(state.player.angle);
				state.player.anglesin = sin(state.player.angle); }
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

	draw3D(state.player, &state.map, &state.pixels, &state.textures);

	/* draw crosshair */
	/* drawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 10, 10, RED); */
	fillRectangle(SCREEN_WIDTH / 2 - 8, SCREEN_HEIGHT / 2 - 1, SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT / 2 + 1, GREEN, &state.pixels);
	fillRectangle(SCREEN_WIDTH / 2 - 1, SCREEN_HEIGHT / 2 - 8, SCREEN_WIDTH / 2 + 1, SCREEN_HEIGHT / 2 + 8, GREEN, &state.pixels);

	SDL_UpdateTexture(state.texture, NULL, &state.pixels, SCREEN_WIDTH * 4);
	SDL_RenderCopy(state.renderer, state.texture, NULL, NULL);
	SDL_RenderPresent(state.renderer);
}


void update() {

	const f32 movespeed = 20.0f * ((f32)state.deltaTime / 1000.0f);
	const u8* keyboardstate = SDL_GetKeyboardState(NULL);


	//TODO: wall collsion checks

	v3 oldPos = state.player.pos;
	u8 moved = 0;

	if (keyboardstate[SDL_SCANCODE_W]) {
		state.player.pos = (v3){
			state.player.pos.x + state.player.anglecos * movespeed,
			state.player.pos.y + state.player.anglesin * movespeed,
			state.player.pos.z
		};
		moved = 1;
	}
	if (keyboardstate[SDL_SCANCODE_S]) {
		state.player.pos = (v3){
			state.player.pos.x - state.player.anglecos * movespeed,
			state.player.pos.y - state.player.anglesin * movespeed,
			state.player.pos.z
		};
		moved = 1;
	}
	if (keyboardstate[SDL_SCANCODE_A]) {
		v2 add = v2Rotate((v2) { state.player.anglecos* movespeed, state.player.anglesin* movespeed}, PI / 2);
		state.player.pos.x += add.x;
		state.player.pos.y += add.y;
		moved = 1;
	}
	if (keyboardstate[SDL_SCANCODE_D]) {
		v2 add = v2Rotate((v2) { state.player.anglecos* movespeed, state.player.anglesin* movespeed }, -PI / 2);
		state.player.pos.x += add.x;
		state.player.pos.y += add.y;
		moved = 1;
	}

	if (moved) {
		Sector curSec = state.map.sectors[state.player.sector - 1];
		for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
			Wall curwall = state.map.walls[i];
			if (curwall.portal > 0 &&
				BOXINTERSECT2D(oldPos.x, oldPos.y, state.player.pos.x, state.player.pos.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) && 
				POINTSIDE2D(state.player.pos.x, state.player.pos.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
				state.player.sector = curwall.portal;
				state.player.pos.z = EYEHEIGHT + state.map.sectors[state.player.sector - 1].zfloor;
				break;
			}
		}
	}
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

	loadLevel();

	state.player.pos = (v3){ 15.0f, 15.0f, 0.0f};
	state.player.sector = 1;
	state.player.pos.z = EYEHEIGHT + state.map.sectors[state.player.sector - 1].zfloor;

	state.player.anglecos = cos(state.player.angle);
	state.player.anglesin = sin(state.player.angle);
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

void loadLevel() {
	FILE* fp = NULL;
	fopen_s(&fp,"level2.txt", "r");
	ASSERT(fp, "error opening leveldata file");
	enum {SECTOR, WALL, NONE} sm = NONE;
	u8 done = 0;

	char line[1024];
	while (fgets(line, sizeof(line), fp) && !done) {
		char*  p = line;
		while (isspace(*p)) { p++; }
		if (!*p || *p == '#') continue;
		if (*p == '{') {
			p++;
			if (*p == 'S') { sm = SECTOR; continue; }
			if (*p == 'W') { sm = WALL; continue; }
			if (*p == 'E')  sm = NONE;
		}
		switch (sm) {
		case SECTOR: {
			Sector* sector = &state.map.sectors[state.map.sectornum++];
			sscanf_s(p, "%d %d %d %f %f", &sector->id, &sector->index, &sector->numWalls, &sector->zfloor, &sector->zceil);
		}
			break;
		case WALL: {
			Wall* wall = &state.map.walls[state.map.wallnum++];
			sscanf_s(p, "%f %f %f %f %d", &wall->a.x, &wall->a.y, &wall->b.x, &wall->b.y, &wall->portal);
		}
			break;
		case NONE:
			done = 1;
			break;
		}
	}
	fclose(fp);
}
