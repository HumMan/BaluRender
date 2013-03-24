#pragma once

static int action=0;
static TVec2 mouse_world_pos;
static float attractor_size=30;
//dummy info
static double phys_time=0;

static const int MAX_BALLS_IN_BLOCK=3;
static const float shear=0.2;
static const float force_factor=75;
static const float ball_rad=0.5;
static const int block_size=1;
static const int room_size=220;
static const int blocks_count=room_size/block_size+1;
static const int balls_count=30000;
static const float gravity=-28;

#define USE_FIXED_POINT

#ifndef USE_FIXED_POINT
typedef float TFloat;
#else
typedef TFixedFloat<int,10> TFloat;
#endif

static TFloat curr_step;

static TVec<TFloat,2> balls_pos[balls_count];
static TVec<TFloat,2> balls_speed[balls_count];
static TVec<TFloat,2> balls_force[balls_count];

static unsigned char grid_count[blocks_count*blocks_count];
static unsigned short grid[blocks_count*blocks_count*MAX_BALLS_IN_BLOCK];

void Collide(int p1, int p2)
{
	TVec<TFloat,2> force;
	TVec<TFloat,2>  delta = balls_pos[p2] - balls_pos[p1];
	TFloat quadlen = delta.SqrLength();
	if(quadlen < TFloat(sqr(ball_rad*2.0f)))
	{	
		if(false)
			force = delta*(quadlen*TFloat(-1.0f));
		else
		{
			//if(quadlen<=TFloat(ball_rad*0.2f))quadlen=TFloat(ball_rad*0.2f);
			TFloat diff=quadlen-TFloat(sqr(ball_rad*2.0f));
			//TFloat diff=sqrt(quadlen)-TFloat(ball_rad*2.0f);
			TVec<TFloat,2> delta_speed=balls_speed[p2]-balls_speed[p1];

			force = delta*(TFloat(force_factor)*diff)
				+ delta_speed*TFloat(TFloat(shear)/*+TFloat(180)/(TFloat(50)+TFloat(20)*balls_pos[p1][1])*/);
			//force*=0.5f;
		}
		balls_speed[p1]+=force;
		balls_speed[p2]-=force;
	}
}
__forceinline void CollideWithCell(int ball, int cell)
{
	for(int s=0;s<grid_count[cell];s++)
	{
		int other_ball=grid[cell*MAX_BALLS_IN_BLOCK+s];
		if(other_ball!=ball)
			Collide(ball,other_ball);
	}
}
void PushBall(int i,int cell_id)
{
	if(grid_count[cell_id]<MAX_BALLS_IN_BLOCK)
	{
		grid[cell_id*MAX_BALLS_IN_BLOCK+grid_count[cell_id]++]=i;
	}
}
void InitGrid()
{
	memset(&grid_count,0,sizeof(grid_count));
	for(int i=0;i<balls_count;i++)
	{
		if(false)
		{
			int cell_x=f2i(balls_pos[i][0]);
			int cell_y=f2i(balls_pos[i][1]);
			int cell_id=cell_y*blocks_count+cell_x;
			if(cell_y>0)
			{
				int temp=cell_id-blocks_count;
				if(cell_x>0)PushBall(i,temp-1);
				PushBall(i,temp);
			}
			if(cell_x>0)
				PushBall(i,cell_id-1);
			PushBall(i,cell_id);
		}
		else
		{

			int cell_id=int(balls_pos[i][0])+int(balls_pos[i][1])*blocks_count;
			if(grid_count[cell_id]<MAX_BALLS_IN_BLOCK)
			{
				grid[cell_id*MAX_BALLS_IN_BLOCK+grid_count[cell_id]++]=i;
			}
		}
	}
	for(int i=0;i<balls_count;i++)
	{
		if(false)
		{
			int cell_x=(balls_pos[i][0]);
			int cell_y=(balls_pos[i][1]);
			int cell_id=cell_y*blocks_count+cell_x;

			if(true)
			{
				TVec<TFloat,2> force(0,0);
				for(int k=cell_x-1;k<=cell_x+1;k++)
					for(int t=cell_y-1;t<=cell_y+1;t++)
					{
						if(k>=0&&k<blocks_count&&t>=0&&t<blocks_count)
							CollideWithCell(i,t*blocks_count+k);
					}
			}else
			{
				CollideWithCell(i,cell_id);
				if(cell_y<blocks_count-1)
				{
					int temp=cell_id+blocks_count;
					CollideWithCell(i,temp);
					if(cell_x>0)CollideWithCell(i,temp-1);
					if(cell_x<blocks_count-1)CollideWithCell(i,temp+1);
				}
				if(cell_x<blocks_count-1)
					CollideWithCell(i,cell_id+1);
				if(cell_x>0)
					CollideWithCell(i,cell_id-1);
			}
		}
		else
		{
#ifndef USE_FIXED_POINT
			int cell_x=f2i(balls_pos[i][0]);
			int cell_y=f2i(balls_pos[i][1]);
#else
			int cell_x=balls_pos[i][0]+TFloat(0.5f);
			int cell_y=balls_pos[i][1]+TFloat(0.5f);
#endif
			int cell_id=cell_y*blocks_count+cell_x;
			TVec<TFloat,2> force(0,0);
			if(cell_y>0)
			{
				int temp=cell_id-blocks_count;
				if(cell_x>0)CollideWithCell(i,temp-1);
				CollideWithCell(i,temp);
			}
			if(cell_x>0)
				CollideWithCell(i,cell_id-1);
			CollideWithCell(i,cell_id);
		}
	}
}
int PosQuery(TVec2 pos)//возвращает первый попавшийся шар наход в позиции pos или -1 если таковой отсутствует
{
	int gr_x=pos[0]/block_size;
	int gr_y=pos[1]/block_size;

	Clamp(0,blocks_count-1,gr_x);
	Clamp(0,blocks_count-1,gr_y);

	int cell_id=gr_y*blocks_count+gr_x;

	for(int i=0;i<grid_count[cell_id];i++)
		if(TVec<TFloat,2>(pos[0],pos[1]).SqrDistance(balls_pos[grid[cell_id*MAX_BALLS_IN_BLOCK+i]])<TFloat(sqr(ball_rad)))
			return grid[cell_id*MAX_BALLS_IN_BLOCK+i];
	return -1;
}

void GetDrawInfo(TVector<TVec2/*TBaluQuad*/>& quads)
{
	for(int i=0;i<=quads.GetHigh();i++)
		//quads[i].Init(TVec2(balls_pos[i][0],balls_pos[i][1]),
		quads[i]=TVec2(balls_pos[i][0],balls_pos[i][1]);/*,
		TVec2(ball_rad,ball_rad),0);*/
}
void UpdateBalls(const float time_step,TTime& time,bool move)
{
	curr_step=TFloat(time_step*1024);
	UINT64 t=time.GetTime();
	TFloat grav_imp=gravity*time_step;
	TVec<TFloat,2> attractor_pos(mouse_world_pos[0],mouse_world_pos[1]);
	if(move)
	{
		for(int i=0;i<balls_count;i++)
		{
			balls_speed[i][1]+=grav_imp;
			if(action!=0)
			{
				TVec<TFloat,2> attractor=attractor_pos-balls_pos[i];
				if(IsIn(attractor[0],TFloat(-attractor_size),TFloat(attractor_size))
					&&IsIn(attractor[1],TFloat(-attractor_size),TFloat(attractor_size)))
				{
					TFloat temp=attractor.SqrLength();
					if(temp<TFloat(sqr(attractor_size))&&temp>TFloat(0.0f))
					{
						balls_speed[i]+=attractor/TFloat(sqrt(temp))*TFloat(action);
					}
				}
			}
			//balls_speed[i]*=TFloat(TFloat(1.0f)-curr_step*TFloat(0.2f));
			balls_speed[i]*=TFloat(0.9999f);
			balls_pos[i]+=balls_speed[i]*curr_step*(1/1024.0f);
			if(balls_pos[i][0]>TFloat(room_size-ball_rad)){
				balls_pos[i][0]=room_size-ball_rad;
				balls_speed[i][0]=0;
			}
			else if(balls_pos[i][0]<TFloat(ball_rad)){
				balls_pos[i][0]=ball_rad;
				balls_speed[i][0]=0;
			}
			if(balls_pos[i][1]>TFloat(room_size-ball_rad)){
				balls_pos[i][1]=room_size-ball_rad;
				balls_speed[i][1]=0;
			}
			else if(balls_pos[i][1]<TFloat(ball_rad)){
				balls_pos[i][1]=ball_rad;
				balls_speed[i][1]=0;
			}
		}
	}	
	InitGrid();	
	phys_time=time.TimeDiff(time.GetTime(),t);
}
