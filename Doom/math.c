#include "math.h"

u8 get_line_intersection(v2 p0, v2 p1, v2 p2, v2 p3, v2* i) {
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
v2 world_pos_to_camera(v2 pos, v2 cam_pos, f32 camsin, f32 camcos) {
	const v2 u = { pos.x - cam_pos.x, pos.y - cam_pos.y};
	return (v2) {
		u.x * camsin - u.y * camcos,
		u.x * camcos + u.y * camsin
	};
}

//player relative pos to absolute one
v2 camera_pos_to_world(v2 pos, v2 cam_pos, f32 camsin, f32 camcos) {
	v2 r = {
		  pos.x * camsin + pos.y * camcos,
		- pos.x * camcos + pos.y * camsin
	};
	return (v2) { r.x + cam_pos.x, r.y + cam_pos.y };
}

//convert angle from [-HFOV/2, HFOV/2] to [0, SCREEN_WIDTH -1] 
i32 screen_angle_to_x(f32 angle) {
	//convert angle to [0.0, 2.0]
	f32 t = 1.0f - tanf(((angle + (HFOV / 2.0f)) / HFOV) * PI_2 - PI_4);
	return (i32)((SCREEN_WIDTH / 2) * t);
}

//convert x from [0, SCREEN_WIDTH -1] to  [-HFOV/2, HFOV/2], only used to fill lookuptable
f32 screen_x_to_angle(i32 x) {
	f32 at = atanf(((-2 * x) / (f32)SCREEN_WIDTH) + 1);
	return (f32)((2 * HFOV * at) / PI);
}

v2 v2_normalize(v2 a) {
	f32 l = v2_len(a);
	return l != 0 ? (v2) { a.x / l, a.y / l } : (v2) { 0, 0 };
}

v2 v2_rotate(v2 a, f32 rot) {
	const v2 oVec = a;
	a.x = oVec.x * cosf(rot) - oVec.y * sinf(rot);
	a.y = oVec.x * sinf(rot) + oVec.y * cosf(rot);
	return a;
}

v2 v2_add(v2 a, v2 b) {
	return (v2) { a.x + b.x, a.y + b.y };
}

v2 v2_sub(v2 a, v2 b) {
	return (v2) { a.x - b.x, a.y - b.y };
}

f32 v2_len(v2 a) {
	return sqrtf(a.x * a.x + a.y * a.y);
}

v2 v2_mul(v2 a, f32 b) {
	return (v2) { a.x* b, a.y* b };
}

f32 ease_in_out_cubic(f32 x) {
    //return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
	return x < 0.5f ? 2.0f * x * x : 1.0f - powf(-2.0f * x + 2.0f, 2.0f) / 2.0f;
}