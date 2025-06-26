#include "plat.h"
#include "map.h"


Platform* activeplats[MAXPLATFORMS];

i32 find_sector_from_tag(i32 tag, i32 sec_start) {
	i32 num_of_sectors = map_get_sectoramt();
	for (i32 i = sec_start; i < num_of_sectors; i++) {
		Sector* sec = map_get_sector_by_idx(i);
		if (sec->tag == tag) return i;
	}
	return -1;
}


void add_plat(Platform* plat) {
	for (size_t i = 0; i < MAXPLATFORMS; i++) {
		if (activeplats[i] == NULL) {
			activeplats[i] = plat;
			return;
		}
	}
	fprintf(stderr, "PLATS FULL\n");
}


void plat_move(Platform* plat) {
	// move plat
	bool done = false;
	switch (plat->status) {
		case UP: {
			done = map_move_sector_plane(plat->sec, plat->speed, plat->high, plat->floor, true);
			break;
		}
		case DOWN: {
			done = map_move_sector_plane(plat->sec, plat->speed, plat->low, plat->floor, false);
			break;
		}
		case WAIT: {
			break;
		}
	}
	// do something when plat finished depending on plat type
	if (done) {
		switch (plat->type) {
			case INFINITE_UP_DOWN: {
				if (plat->status == UP) plat->status = DOWN;
				else plat->status = UP;

				break;
			}
			case RAISE_STAIRS: {
				if (plat->tick.function != (actionf)(-1)) ticker_remove(&plat->tick);
				plat->sec->specialdata = NULL;
				plat->sec->tag = 0;
				*plat = (Platform){ 0 };
				free(plat);
				break;
			}
		}
	}
}


void try_reverse_move(Sector* sec, plat_type type, bool floor) {
	Platform* old_plat = (Platform*)sec->specialdata;
	if (old_plat->type != type || !old_plat->reverseable) return;

	if (old_plat->status == UP)  old_plat->status = DOWN; 
	else if (old_plat->status == DOWN)  old_plat->status = UP; 
}


// creates a plat for a sector, sector_search_start_index and height are only used sometimes
void create_plat(i32 tag, plat_type type, bool floor, i32 sector_search_start_index, f32 height) {
	// tag 0 and below have no effect
	if (tag <= 0) return;
	Platform* plat;
	i32 secnum = sector_search_start_index;
	Sector* sec;

	while ((secnum = find_sector_from_tag(tag, secnum)) >= 0) {
		sec = map_get_sector_by_idx(secnum);
		// if sector already has an action try to reverse it
		if (sec->specialdata) {
			try_reverse_move(sec, type, floor);
			return;
		}

		plat = malloc(sizeof(Platform));
		if (!plat) return;

		plat->type = type;
		plat->sec = sec;
		plat->sec->specialdata = plat;
		plat->floor = floor;
		plat->tick.function = (actionf) plat_move;
		ticker_add(&plat->tick);

		switch (type) {
			case INFINITE_UP_DOWN: {
				plat->speed = 10.0f;
				plat->low = sec->zfloor;
				plat->high = sec->zceil;
				plat->status = UP;
				plat->reverseable = true;
				break;
			}
			case RAISE_STAIRS: {
				// recursivly create more plats for neighbouring sector to also raise, but each a incrementally higher
				f32 new_height = height + 2.0f;
				create_plat(tag, type, floor, secnum + 1, new_height);
				plat->speed = 1.0f;
				plat->high = new_height + sec->zfloor;
				plat->status = UP;
				plat->reverseable = false;
				break;
			}
		}
		add_plat(plat);
		break;
	}
}