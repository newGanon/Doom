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
void sortWalls();
u8 pointInsideSector(Map* map, i32 sec, v2 p);


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
				f32 rotaionspeed = 0.001f;
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

	sortWalls();

	const f32 movespeed = 20.0f * ((f32)state.deltaTime / 1000.0f);
	const f32 gravity = -80.0f * ((f32)state.deltaTime / 1000.0f);
	const u8* keyboardstate = SDL_GetKeyboardState(NULL);
	Player* p = &state.player;
	Map* map = &state.map;
	Sector curSec = state.map.sectors[p->sector - 1];

	//vertical collision detection
	if (keyboardstate[SDL_SCANCODE_SPACE] && !p->inAir) {
		p->inAir = 1;
		p->velocity.z = 25.0f;
	}

	if (p->inAir) {
		p->velocity.z += gravity;
		f32 dvel = p->velocity.z * ((f32)state.deltaTime / 1000.0f);
		//floor collision
		if (p->velocity.z < 0 && (p->pos.z + dvel) < (curSec.zfloor + EYEHEIGHT)) {
			p->velocity.z = 0;
			p->inAir = 0;
			p->pos.z = (curSec.zfloor + EYEHEIGHT);
		}
		//ceiling collision
		else if (p->velocity.z > 0 && (p->pos.z + dvel) > (curSec.zceil - HEADMARGIN)) {
			p->velocity.z = 0;
			p->pos.z = curSec.zceil - HEADMARGIN;
		}
		//if no collision was detected just add the velocity
		else {
			p->pos.z += dvel;
		}
	}

	//horizontal collision detection
	v2 dpos = { 0, 0 };

	if (keyboardstate[SDL_SCANCODE_W]) { dpos.x += p->anglecos; dpos.y += p->anglesin; }
	if (keyboardstate[SDL_SCANCODE_S]) { dpos.x -= p->anglecos; dpos.y -= p->anglesin; }
	if (keyboardstate[SDL_SCANCODE_A]) { dpos.x -= p->anglesin; dpos.y += p->anglecos; }
	if (keyboardstate[SDL_SCANCODE_D]) { dpos.x += p->anglesin; dpos.y -= p->anglecos; }

	u8 moved = keyboardstate[SDL_SCANCODE_W] || keyboardstate[SDL_SCANCODE_S] || keyboardstate[SDL_SCANCODE_A] || keyboardstate[SDL_SCANCODE_D];

	f32 acceleration = moved ? 0.4 : 0.3;

	p->velocity.x = p->velocity.x * (1 - acceleration) + dpos.x * acceleration * movespeed;
	p->velocity.y = p->velocity.y * (1 - acceleration) + dpos.y * acceleration * movespeed;

	//check for collision and if player entered new sector
	//TODO: fix hack that loops 2 times
	i32 wallind = -1;
	Sector oldSec = curSec;
	u8 oldinAir = p->inAir;
	f32 oldz = p->pos.z;
	u8 hitPortal = 0;
	Sector newSec;
	for (u8 t = 0; t < 2; t++){
		for (i32 i = curSec.index; i < curSec.index + curSec.numWalls; i++) {
			Wall curwall = map->walls[i];
			if (BOXINTERSECT2D(p->pos.x, p->pos.y, p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) &&
				POINTSIDE2D(p->pos.x + p->velocity.x, p->pos.y + p->velocity.y, curwall.a.x, curwall.a.y, curwall.b.x, curwall.b.y) > 0)) {
					f32 stepl = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zfloor : 10e10;
					f32 steph = curwall.portal > 0 ? map->sectors[curwall.portal - 1].zceil : -10e10;
					//collision with wall, top or lower part of portal
					if (stepl > p->pos.z - EYEHEIGHT + STEPHEIGHT ||
						steph < p->pos.z + HEADMARGIN) {
						if (wallind != -1) {
							p->velocity.x = 0;
							p->velocity.y = 0;
							curSec = oldSec;
							p->sector = oldSec.id;
							p->inAir = oldinAir;
							p->pos.z = oldz;
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
					else if (curwall.portal > 0) {
						hitPortal = 1;
						newSec = state.map.sectors[curwall.portal - 1];
					}
			}
		}
		if (hitPortal) {
			t = 0;
			hitPortal = 0;
			curSec = newSec;
			p->sector = curSec.id;
			if (p->pos.z < EYEHEIGHT + curSec.zfloor) p->pos.z = EYEHEIGHT + curSec.zfloor;
			else if (p->pos.z > EYEHEIGHT + curSec.zfloor) p->inAir = 1;
		}
	}

	//check if point is outside
	/*if (pointInsideSector(&state.map, p->sector, (v2) { p->pos.x + p->velocity.x, p->pos.y + p->velocity.y })) {
		p->pos.x += p->velocity.x;
		p->pos.y += p->velocity.y;
	}*/

	p->pos.x += p->velocity.x;
	p->pos.y += p->velocity.y;

	//reset player pos
	if (keyboardstate[SDL_SCANCODE_R]) { state.player.pos = (v3){ 15.0f, 15.0f, EYEHEIGHT + state.map.sectors[0].zfloor }; state.player.sector = 1; }
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
	state.player.inAir = 0;

	state.player.angle = PI_2;
	state.player.anglecos = cos(state.player.angle);
	state.player.anglesin = sin(state.player.angle);

	//init global render variables 
	//yslope if camera had z position 1, this value is later scaled by the actual height value
	for (i32 y = 0; y < SCREEN_HEIGHT; y++) {
		f32 dy = y - SCREEN_HEIGHT / 2;
		yslope[y] = (SCREEN_HEIGHT * 0.5f) / dy;
	}
	yslope[360] = 1000;

	for (i32 x = 0; x < SCREEN_WIDTH; x++) {
		screenxtoangle[x] = screen_x_to_angle(x);
	}
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
	char textureFileNames[2][50] = {"test.bmp", "spritetest2.bmp"};
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
	fopen_s(&fp,"level4.txt", "r");
	ASSERT(fp, "error opening leveldata file");
	enum {SECTOR, WALL, NONE} sm = NONE;
	u8 done = 0;

	char line[1024];
	while (fgets(line, sizeof(line), fp) && !done) {
		char* p = line;
		while (isspace(*p)) { p++; }
		if (!*p || *p == '#') continue;
		if (*p == '{') {
			p++;
			if (*p == 'S') { sm = SECTOR; continue; }
			if (*p == 'W') { sm = WALL; continue; }
			if (*p == 'E')  sm = NONE;
			else sm = NONE;
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


u8 pointInsideSector(Map* map, i32 sec, v2 p) {
	Sector s = map->sectors[sec - 1];
	for (i32 i = s.index; i < (s.index + s.numWalls); i++) {
		Wall w = map->walls[i];
		if (POINTSIDE2D(p.x, p.y, w.a.x, w.a.y, w.b.x, w.b.y) > 0) {
			return 0;
		}
	}
	return 1;
}

void sortWalls() {
	Map* map = &state.map;
	//calc distances
	for (i32 wallind= 0; wallind < map->wallnum; wallind++)
	{
		v2 p1 = world_pos_to_camera(map->walls[wallind].a, state.player);
		v2 p2 = world_pos_to_camera(map->walls[wallind].b, state.player);

		map->walls[wallind].distance = (p1.y + p2.y) / 2;
	}
	//sort wall with distance from player
	for (i32 secind = 0; secind < map->sectornum; secind++)
	{
		Sector sec = map->sectors[secind];
		for (i32 step = sec.index; step < sec.index + sec.numWalls; step++)
		{
			for (i32 i = sec.index; i < sec.index + sec.numWalls - 1; i++)
			{
				if (map->walls[i].distance > map->walls[i + 1].distance) {
					Wall tempWall = map->walls[i];
					map->walls[i] = map->walls[i + 1];
					map->walls[i+1] = tempWall;
				}
			}
		}
	}
}