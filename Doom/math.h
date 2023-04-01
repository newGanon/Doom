#pragma once
#include "util.h"

#define SIGN(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))

i32 get_line_intersection(v2 p0, v2 p1, v2 p2, v2 p3, v2* i);
v2 world_pos_to_camera(v2 pos, Player player);
i32 screen_angle_to_x(f32 angle);

v2 v2Normalize(v2 a);
v2 v2Rotate(v2 a, f32 rot);
v2 v2Add(v2 a, v2 b);
v2 v2Sub(v2 a, v2 b);
v2 v2Mul(v2 a, f32 b);
f32 v2Len(v2 a);
f32 clamp(f32 d, f32 min, f32 max);

