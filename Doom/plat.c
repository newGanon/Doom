#include "plat.h"


i32 find_sector_from_tag(tag, secnum) {
	return -1;
}


void create_plat(i32 tag, PLAT_TYPE type) {
	Platform* plat;
	i32 secnum = -1;
	Sector* sec;

	while ((secnum = find_sector_from_tag(tag, secnum)) >= 0) {
		break;
	}
}