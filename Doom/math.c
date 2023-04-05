#include "math.h"
#include <math.h>

i32 get_line_intersection(v2 p0, v2 p1, v2 p2, v2 p3, v2* i) {
	v2 s1 = { p1.x - p0.x, p1.y - p0.y };
	v2 s2 = { p3.x - p2.x, p3.y - p2.y };

	f32 s, t;
	s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / (-s2.x * s1.y + s1.x * s2.y);
	t = (s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / (-s2.x * s1.y + s1.x * s2.y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
		if (i != NULL) {
			i->x = p0.x + (t * s1.x);
			i->y = p0.y + (t * s1.y);
		}
		return 1;
	}
	return 0;
}

//absolute pos to player relative pos
v2 world_pos_to_camera(v2 pos, Player player) {
	const v2 u = { pos.x - player.pos.x, pos.y - player.pos.y};
	return (v2) {
		u.x * player.anglesin - u.y * player.anglecos,
		u.x * player.anglecos + u.y * player.anglesin
	};
}
//player relative pos to absolute one
v2 camera_pos_to_world(v2 pos, Player player) {
	v2 r = {
		pos.x * player.anglesin + pos.y * player.anglecos,
		- pos.x * player.anglecos + pos.y * player.anglesin
	};
	return (v2) { r.x + player.pos.x, r.y + player.pos.y };
}

//convert angle from [-HFOV/2, HFOV/2] to [0, SCREEN_WIDTH -1] 
i32 screen_angle_to_x(f32 angle) {
	//convert angle to [0.0, 2.0]
	f32 t = 1.0f - tan(((angle + (HFOV / 2.0)) / HFOV) * PI_2 - PI_4);
	return ((i32)(SCREEN_WIDTH / 2)) * t;
}

//convert x from [0, SCREEN_WIDTH -1] to  [-HFOV/2, HFOV/2]
f32 screen_x_to_angle(i32 x) {
	f32 at = atan(((-2 * x) / (f32)SCREEN_WIDTH) + 1);
	return (f32)((2 * HFOV * at) / PI);
}

v2 v2Normalize(v2 a) {
	f32 l = v2Len(a);
	return l != 0 ? (v2) { a.x / l, a.y / l } : (v2) { 0, 0 };
}

v2 v2Rotate(v2 a, f32 rot) {
	const v2 oVec = a;
	a.x = oVec.x * (f32)cos(rot) - oVec.y * (f32)sin(rot);
	a.y = oVec.x * (f32)sin(rot) + oVec.y * (f32)cos(rot);
	return a;
}

v2 v2Add(v2 a, v2 b) {
	return (v2) { a.x + b.x, a.y + b.y };
}

v2 v2Sub(v2 a, v2 b) {
	return (v2) { a.x - b.x, a.y - b.y };
}

f32 v2Len(v2 a) {
	return (f32)sqrt(a.x * a.x + a.y * a.y);
}

v2 v2Mul(v2 a, f32 b) {
	return (v2) { a.x* b, a.y* b };
}

f32 clamp(f32 d, f32 min, f32 max) {
	const f32 t = d < min ? min : d;
	return t > max ? max : t;
}