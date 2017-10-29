#pragma once

#include <baluLib.h>

extern "C" {
#include "../glfw/deps/tinycthread.h"
}

const int MAX_BALLS_IN_BLOCK = 2;
const float ball_rad = 0.5;
const int block_size = 1;

#define USE_COLOR
//#define USE_GRIDLOCK
#define D3
#ifdef D1
const int room_size = 125;
const int balls_count = 7000;
const int point_size = 4;
#endif

#ifdef D2
const int room_size = 255;
const int balls_count = 30000;
const int point_size = 3;
#endif

#ifdef D3
const int room_size = 355;
const int balls_count = 60000;
const int point_size = 2;
#endif

#ifdef D4
const int room_size = 455;
const int balls_count = 200000;
const int point_size = 1;
#endif

const int blocks_count = room_size / block_size + 1;
const float gravity = 0.1;
const float global_damp = 0.999f;

const int fracture_part = 12;
const int ball_pos_draw_precision = 6;//-1 ... +8 больше - точнее

typedef BaluLib::TFixedFloat<int, fracture_part> TFloat;
typedef BaluLib::TVec<TFloat, 2> TVec2_Float;

void UpdateBalls(bool move);

void SendBallsPos(BaluLib::TVec<short, 2>* points);

void InitBalls(int threads_count);

void CopyPosAndColor(TVec2_Float *pos, BaluLib::TVec<unsigned char, 4> *color);

TVec2_Float* GetBallsPosArray();
BaluLib::TVec<unsigned char, 4>* GetBallsColorArray();

void ChangeAttractor(double offset);

void SetAction(int value);

void SetAttractorPos(BaluLib::TVec2 mouse_world_pos);

void ChangeThreadsCount(int new_count);