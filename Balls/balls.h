#pragma once


static const int threads_count = 1;
static struct TThreadInfo
{
	//HANDLE broadphase_event;
	//HANDLE end_broadphase_event;

	//HANDLE integrate_event;
	//HANDLE end_integrate_event;

	bool is_main_thread;
	int offset, high;
}threads[threads_count];

static int action = 0;
static TVec2 mouse_world_pos;
static float attractor_size = 30;

static double phys_time = 0;

static const int MAX_BALLS_IN_BLOCK = 2;
static const float ball_rad = 0.5;
static const int block_size = 1;

#define USE_COLOR
#define D3

#ifdef D1
static const int room_size = 125;
static const int balls_count = 7000;
static const int point_size = 4;
#endif

#ifdef D2
static const int room_size = 255;
static const int balls_count = 30000;
static const int point_size = 3;
#endif

#ifdef D3
static const int room_size = 355;
static const int balls_count = 60000;
static const int point_size = 2;
#endif

#ifdef D4
static const int room_size = 455;
static const int balls_count = 200000;
static const int point_size = 1;
#endif

static const int balls_on_thread = balls_count / threads_count;
static const int blocks_count = room_size / block_size + 1;
static const float gravity = 0.1;
static const float global_damp = 0.999f;

static const int fracture_part = 12;
static const int ball_pos_draw_precision = 6;//-1 ... +8 больше - точнее

typedef TFixedFloat<int, fracture_part> TFloat;
typedef TVec<TFloat, 2> TVec2_Float;

static unsigned char grid_count[blocks_count*blocks_count];
static int grid[blocks_count*blocks_count*MAX_BALLS_IN_BLOCK];
static TVec2_Float balls_pos[balls_count];
static TVec2_Float balls_speed[balls_count];

static TVec<unsigned char, 4> balls_color[balls_count];

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

void BroadPhase(void* p)
{
	TThreadInfo* params = (TThreadInfo*)p;
	//do
	{
		//if (!params->is_main_thread)
		//	WaitForSingleObject(params->broadphase_event, INFINITE);
		for (int i = params->offset; i<params->high; i++)
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
		//if (!params->is_main_thread)
		//{
		//	ResetEvent(params->broadphase_event);
		//	SetEvent(params->end_broadphase_event);
		//}
	} //while (!params->is_main_thread);
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

void UpdateBalls(TTime& time, bool move)
{
	auto t = time.GetTime();
	TVec2_Float attractor_pos(mouse_world_pos[0], mouse_world_pos[1]);
	if (move)
	{
		for (int i = 0; i<balls_count; i++)
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
			if (balls_pos[i][0]>TFloat(room_size - ball_rad)){
				balls_pos[i][0] = room_size - ball_rad - 0.01f;
				balls_speed[i][0] = 0;
			}
			else if (balls_pos[i][0]<TFloat(ball_rad)){
				balls_pos[i][0] = ball_rad + 0.01f;
				balls_speed[i][0] = 0;
			}
			if (balls_pos[i][1]>TFloat(room_size - ball_rad)){
				balls_pos[i][1] = room_size - ball_rad - 0.01f;
				balls_speed[i][1] = 0;
			}
			else if (balls_pos[i][1]<TFloat(ball_rad)){
				balls_pos[i][1] = ball_rad + 0.01f;
				balls_speed[i][1] = 0;
			}
		}
	}
	InitGrid();

	//for (int i = 1; i<threads_count; i++)
	//	SetEvent(threads[i].broadphase_event);

	BroadPhase(&threads[0]);

	//HANDLE events[threads_count - 1];//TODO  для одного потока не работает
	//for (int i = 0; i<threads_count - 1; i++)
	//	events[i] = threads[i + 1].end_broadphase_event;

	//if (threads_count>1)
	//	WaitForMultipleObjects(threads_count - 1, &events[0], TRUE, INFINITE);

	//for (int i = 0; i<threads_count - 1; i++)
	//	ResetEvent(threads[i + 1].end_broadphase_event);

	//перераспределяем шары
	if (true)
	{
		static int dd = 0;
		dd++;
		if (dd % 7 == 0)
			RedistrBalls();
	}
	phys_time = time.TimeDiff(time.GetTime(), t);
}

void SendBallsPos(TVec<short, 2>* points)
{
	for (int i = 0; i<balls_count; i++)
		points[i] = TVec<short, 2>(
		balls_pos[i][0].GerRAW() >> (fracture_part - ball_pos_draw_precision),
		balls_pos[i][1].GerRAW() >> (fracture_part - ball_pos_draw_precision));
}
