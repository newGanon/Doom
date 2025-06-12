#include "plat.h"


Platform* activeplats[MAXPLATFORMS];

i32 find_sector_from_tag(i32 tag, i32 sec_start) {
	i32 num_of_sectors = get_sectoramt();
	for (size_t i = sec_start; i < num_of_sectors; i++) {
		Sector* sec = get_sector(i);
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
			done = move_sector_plane(plat->sec, plat->speed, plat->high, plat->floor, true);
			break;
		}
		case DOWN: {
			done = move_sector_plane(plat->sec, plat->speed, plat->low, plat->floor, false);
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
		}
	}
}

void try_reverse_move(Sector* sec, plat_type type, bool floor) {
	Platform* old_plat = (Platform*)sec->specialdata;
	if (old_plat->type != type || !old_plat->reverseable) return;

	if (old_plat->status == UP)  old_plat->status = DOWN; 
	else if (old_plat->status == DOWN)  old_plat->status = UP; 
}


void create_plat(i32 tag, plat_type type, bool floor) {
	// tag 0 and below have no effect
	if (tag <= 0) return;
	Platform* plat;
	i32 secnum = 0;
	Sector* sec;

	while ((secnum = find_sector_from_tag(tag, secnum)) >= 0) {
		sec = get_sector(secnum);
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
		}
		add_plat(plat);
		break;
	}
}