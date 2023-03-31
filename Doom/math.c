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