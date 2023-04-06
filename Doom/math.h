#pragma once
#include "util.h"

#define SIGN(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a < b ? b : a)
#define VETORCROSSPROD2D(x0, x1, y0, y2) ((x0 * y1) - (y0 * x1))
#define OVERLAP1D(a0, a1, b0, b1) (MIN(a0,a1) <= MAX(b0,b1) && MIN(b0,b1) <= MAX(a0,a1))
#define BOXINTERSECT2D(x0, y0, x1, y1, x2, y2, x3, y3) ((OVERLAP1D(x0, x1, x2, x3) && OVERLAP1D(y0, y1, y2, y3))
#define POINTSIDE2D(px, py, x0, y0, x1, y1) ((x1 - x0) * (py - y0) - (y1 - y0) * (px - x0)) 

i32 get_line_intersection(v2 p0, v2 p1, v2 p2, v2 p3, v2* i);
v2 world_pos_to_camera(v2 pos, Player player);
v2 camera_pos_to_world(v2 pos, Player player);
i32 screen_angle_to_x(f32 angle);
f32 screen_x_to_angle(i32 x);

v2 v2Normalize(v2 a);
v2 v2Rotate(v2 a, f32 rot);
v2 v2Add(v2 a, v2 b);
v2 v2Sub(v2 a, v2 b);
v2 v2Mul(v2 a, f32 b);
f32 v2Len(v2 a);
f32 clamp(f32 d, f32 min, f32 max);

