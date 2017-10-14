#pragma once

#include "balls.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <atomic>

using namespace BaluLib;

struct TThreadInfo
{
	std::atomic_flag start_pos_update;
	std::atomic_flag start_broadphase;

	volatile bool finished;

	bool is_main_thread;
	int offset, high;

	thrd_t physics_thread;

	TThreadInfo()
	{
		start_pos_update.test_and_set();
		start_broadphase.test_and_set();
		finished = false;
		is_main_thread = false;
		offset = 0;
		high = 0;
	}
};

TThreadInfo* threads;

//TThreadInfo threads[3];

unsigned char grid_count[blocks_count*blocks_count];

//static std::atomic_flag grid_lock[blocks_count*blocks_count];

int grid[blocks_count*blocks_count*MAX_BALLS_IN_BLOCK];
TVec2_Float balls_pos[balls_count];
TVec2_Float balls_speed[balls_count];

TVec<unsigned char, 4> balls_color[balls_count];

int action = 0;
TVec2 mouse_world_pos;
float attractor_size = 30;
int threads_count = 3;
int new_threads_count = 0;

bool test_broadphase = false;

bool changing_threads_count = false;

void SetAttractorPos(TVec2 attractor_pos)
{
	mouse_world_pos = attractor_pos;
}

void SetAction(int value)
{
	action = value;
}

void ChangeAttractor(double offset)
{
	attractor_size += offset;
	Clamp(0.0f, room_size / 2.0f, attractor_size);
	attractor_size = Clamp(0.01f, 1000.0f, attractor_size);
}

TVec2_Float* GetBallsPosArray()
{
	return balls_pos;
}

TVec<unsigned char, 4>* GetBallsColorArray()
{
	return balls_color;
}

inline void Collide(int p1, int p2)
{
	TVec2_Float  delta = balls_pos[p2] - balls_pos[p1];
	TFloat quadlen = delta.SqrLength();
	if (quadlen < TFloat(sqr(ball_rad*2.0f)))
	{
		TFloat diff = (quadlen - TFloat(sqr(ball_rad*2.0f)));
		TVec2_Float delta_speed = balls_speed[p2] - balls_speed[p1];

		delta_speed >>= 3;
		diff <<= 6;
		TVec2_Float force = delta*diff + delta_speed;

		balls_speed[p1] += force;
		balls_speed[p2] -= force;
	}
}
inline void CollideWithCell(int ball, int cell)
{
	//while (grid_lock[cell].test_and_set(std::memory_order_acquire));
	if (MAX_BALLS_IN_BLOCK>2)
	{
		for (int s = 0; s<grid_count[cell]; s++)
		{
			int other_ball = grid[cell*MAX_BALLS_IN_BLOCK + s];
			if (other_ball != ball)
				Collide(ball, other_ball);
		}
	}
	else
	{
		int count = grid_count[cell];
		if (count >= 1)
		{
			int other_ball = grid[cell*MAX_BALLS_IN_BLOCK];
			if (other_ball != ball)
				Collide(ball, other_ball);
			if (count >= 2)
			{
				other_ball = grid[cell*MAX_BALLS_IN_BLOCK + 1];
				if (other_ball != ball)
					Collide(ball, other_ball);
			}
		}
	}
	//grid_lock[cell].clear(std::memory_order_release);
}

inline void FindBallIndex(int start_block, int index, int& block, int& block_offset)
{
	int cell_id = int(balls_pos[index][1])*blocks_count + int(balls_pos[index][0]);
	block = cell_id;

	if (MAX_BALLS_IN_BLOCK>2)
	{
		for (int s = 0; s<grid_count[cell_id]; s++)
		{
			int b = grid[cell_id*MAX_BALLS_IN_BLOCK + s];
			if (b == index)
			{
				block_offset = s;
				return;
			}
		}
	}
	else
	{
		int count = grid_count[cell_id];
		if (count >= 1)
		{
			if (grid[cell_id*MAX_BALLS_IN_BLOCK] == index)
			{
				block_offset = 0;
				return;
			}
			if (count >= 2)
			{
				if (grid[cell_id*MAX_BALLS_IN_BLOCK + 1] == index)
				{
					block_offset = 1;
					return;
				}
			}
		}
	}
}

void RedistrBalls()
{
	int last_id = -1;
	for (int i = 0; i<(blocks_count*blocks_count); i++)
	{
		for (int s = 0; s<grid_count[i]; s++)
		{
			int b = grid[i*MAX_BALLS_IN_BLOCK + s];
			if (b == last_id + 1)last_id = b;
			else
			{
				int block = -1;
				int block_offset = -1;
				while (block_offset == -1)
				{
					last_id++;
					assert(last_id<balls_count);
					FindBallIndex(i, last_id, block, block_offset);
				}
				int other_ball = last_id;

				assert(other_ball<balls_count&&other_ball >= 0);
				assert(b<balls_count&&b >= 0);
				Swap(balls_color[b], balls_color[other_ball]);
				Swap(balls_pos[b], balls_pos[other_ball]);
				Swap(balls_speed[b], balls_speed[other_ball]);

				int g1 = i*MAX_BALLS_IN_BLOCK + s;
				int g2 = block*MAX_BALLS_IN_BLOCK + block_offset;
				assert(g1 >= 0 && g1<blocks_count*blocks_count*MAX_BALLS_IN_BLOCK);
				assert(g2 >= 0 && g2<blocks_count*blocks_count*MAX_BALLS_IN_BLOCK);
				Swap(grid[g1], grid[g2]);
			}
		}
	}
}

void BroadPhase(int offset, int high)
{
	for (int i = offset; i <= high; i++)
	{
		int cell_x = balls_pos[i][0] + TFloat(0.5f);
		int cell_y = balls_pos[i][1] + TFloat(0.5f);
		int cell_id = cell_y*blocks_count + cell_x;

		if (cell_y>0)
		{
			int temp = cell_id - blocks_count;
			if (cell_x>0)CollideWithCell(i, temp - 1);
			CollideWithCell(i, temp);
		}
		if (cell_x>0)
			CollideWithCell(i, cell_id - 1);
		CollideWithCell(i, cell_id);
	}
}


void InitGrid()
{
	memset(&grid_count, 0, sizeof(grid_count));
	for (int i = 0; i<balls_count; i++)
	{
		int cell_id = int(balls_pos[i][0]) + int(balls_pos[i][1])*blocks_count;
		if (grid_count[cell_id]<MAX_BALLS_IN_BLOCK)
		{
			grid[cell_id*MAX_BALLS_IN_BLOCK + (grid_count[cell_id]++)] = i;
		}
	}
}

void UpdateBallsPos(int offset, int hight)
{
	TVec2_Float attractor_pos(mouse_world_pos[0], mouse_world_pos[1]);
	for (int i = offset; i <= hight; i++)
	{
		balls_speed[i][1] += TFloat(-gravity);
		if (action != 0)
		{
			TVec2_Float attractor = attractor_pos - balls_pos[i];
			if (IsIn(attractor[0], TFloat(-attractor_size), TFloat(attractor_size))
				&& IsIn(attractor[1], TFloat(-attractor_size), TFloat(attractor_size)))
			{
				if (true)
				{
					attractor >>= 4;
					attractor *= action;
					balls_speed[i] += attractor;
				}
				else
				{
					TFloat temp = attractor.SqrLength();
					if (temp<TFloat((float)sqr(attractor_size)) && temp>TFloat(0.0f))
					{
						balls_speed[i] += attractor / TFloat((float)sqrt(temp))*TFloat(action*3.0f);

					}
				}
			}
		}
		balls_speed[i] -= balls_speed[i] >> 11;

		balls_pos[i] += balls_speed[i] >> 8;
		if (balls_pos[i][0]>TFloat(room_size - ball_rad)) {
			balls_pos[i][0] = room_size - ball_rad - 0.01f;
			balls_speed[i][0] = 0;
		}
		else if (balls_pos[i][0]<TFloat(ball_rad)) {
			balls_pos[i][0] = ball_rad + 0.01f;
			balls_speed[i][0] = 0;
		}
		if (balls_pos[i][1]>TFloat(room_size - ball_rad)) {
			balls_pos[i][1] = room_size - ball_rad - 0.01f;
			balls_speed[i][1] = 0;
		}
		else if (balls_pos[i][1]<TFloat(ball_rad)) {
			balls_pos[i][1] = ball_rad + 0.01f;
			balls_speed[i][1] = 0;
		}
	}
}


int PhysThread(void* p)
{
	TThreadInfo* params = (TThreadInfo*)p;
	assert(!params->is_main_thread);
	do
	{
		while (params->start_pos_update.test_and_set(std::memory_order_acquire))
		{
			thrd_yield();
			if (changing_threads_count)
			{
				return 0;
			}
		}

		if (!test_broadphase)
			UpdateBallsPos(params->offset, params->high);

		params->finished = true;

		while (params->start_broadphase.test_and_set(std::memory_order_acquire));

		BroadPhase(params->offset, params->high);

		params->finished = true;

	} while (true);

	return 0;
}

void WaitForAllFinish()
{
	bool finished = false;
	do
	{
		finished = true;
		for (int i = 0; i < threads_count - 1; i++)
			finished &= threads[i + 1].finished;
	} while (!finished);

	for (int i = 0; i < threads_count - 1; i++)
		threads[i + 1].finished = false;
}

void InitThreads();

void ChangeThreadsCount()
{
	changing_threads_count = true;
	for (int i = 1; i < threads_count; i++)
	{
		int res;
		thrd_join(&threads[i].physics_thread, &res);
	}
	changing_threads_count = false;

	threads_count = new_threads_count;

	delete threads;

	InitThreads();
}

void UpdateBalls(bool test_broadphase_value)
{
	if (new_threads_count != threads_count)
	{
		ChangeThreadsCount();
	}

	test_broadphase = test_broadphase_value;

	//start all processing threads
	for (int i = 1; i < threads_count; i++)
		threads[i].start_pos_update.clear(std::memory_order_release);

	if (!test_broadphase)
	{
		UpdateBallsPos(threads[0].offset, threads[0].high);
	}

	WaitForAllFinish();

	InitGrid();

	//start all processing threads
	for (int i = 1; i < threads_count; i++)
		threads[i].start_broadphase.clear(std::memory_order_release);

	BroadPhase(threads[0].offset, threads[0].high);

	WaitForAllFinish();

	//перераспределяем шары для более эффективного кэширования
	if (true)
	{
		static unsigned int phys_frame = 0;
		phys_frame++;
		if (phys_frame % 7 == 0)
			RedistrBalls();
	}
}

void SendBallsPos(TVec<short, 2>* points)
{
	for (int i = 0; i<balls_count; i++)
		points[i] = TVec<short, 2>(
			balls_pos[i][0].GerRAW() >> (fracture_part - ball_pos_draw_precision),
			balls_pos[i][1].GerRAW() >> (fracture_part - ball_pos_draw_precision));
}

void InitThreads()
{
	threads = new TThreadInfo[threads_count];

	int balls_on_thread = balls_count / threads_count;

	threads[0].offset = 0;
	threads[0].is_main_thread = true;
	threads[0].high = threads[0].offset + balls_on_thread - 1;
	for (int i = 1; i<threads_count; i++)
	{
		threads[i].is_main_thread = false;
		threads[i].finished = false;
		threads[i].offset = i*balls_on_thread;
		if (i == threads_count - 1)
			threads[i].high = balls_count - 1;
		else
			threads[i].high = threads[i].offset + balls_on_thread - 1;

		thrd_create(&threads[i].physics_thread, PhysThread, &threads[i]);
	}
}

void ChangeThreadsCount(int new_count)
{
	new_threads_count = Clamp(1, 255, new_count);
}

void InitBalls(int new_threads_count_value)
{
	new_threads_count = new_threads_count_value;
	threads_count = new_threads_count_value;

	InitThreads();

	for (int i = 0; i<balls_count; i++)
	{
		int balls_on_line = room_size / (ball_rad * 2 + 0.01);
		if (false)
		{
			balls_pos[i] =
				TVec2_Float(
				(short int)(i%balls_on_line),
					(short int)(i / balls_on_line)) + TVec2_Float(0.5f, 0.5f)*(1 + Randfs()*0.001f);
		}
		else
		{
			balls_pos[i] = TVec2_Float(0.5f + Randf()*(room_size - 1), 0.5f + Randf()*(room_size - 1));
		}
	}

	for (int i = 0; i<balls_count; i++)
	{
		balls_color[i] = TVec<unsigned char, 4>(
			255 * sinf(balls_pos[i][1] * TFloat(70.0f / room_size)),
			int(balls_pos[i][0] * TFloat(255.0f / room_size)),
			int(balls_pos[i][1] * TFloat(255.0f / room_size)),
			255);
	}
}

void CopyPosAndColor(TVec2_Float *pos, TVec<unsigned char, 4> *color)
{
	memcpy(pos, balls_pos, sizeof(balls_pos));
	memcpy(color, balls_color, sizeof(balls_color));
}