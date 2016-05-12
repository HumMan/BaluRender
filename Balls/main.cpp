#if defined(_MSC_VER)
// Make MS math.h define M_PI
#define _USE_MATH_DEFINES
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

extern "C" {
#include "../glfw/deps/tinycthread.h"
}

#include <GLFW/glfw3.h>

#include "baluRender.h"

#include "balls.h"

#if defined(WIN32)||defined(_WIN32)
#else
#define sprintf_s sprintf
#endif

using namespace BaluRender;

TBaluRender* render;


// Window dimensions
float aspect_ratio;

// Thread synchronization
struct {
	double    t;         // Time (s)
	float     dt;        // Time since last frame (s)
	cnd_t     p_done;    // Condition: particle physics done
	cnd_t     d_done;    // Condition: particle draw done
	mtx_t     particles_lock; // Particles data sharing mutex
} thread_sync;

TVec2 cursor_pos;

bool balls_pause = false;
bool test_broadphase = false;

TTime balu_time;

using namespace TBaluRenderEnums;

TVertexBufferId pos_buff;

#ifdef USE_COLOR
TVertexBufferId color_buff;
#endif

TMatrix<float, 4> ortho_m, ortho_m_inv;

double draw_time;

TStreamsDesc streams;

void Init()
{

	{
		threads[0].offset = 0;
		threads[0].is_main_thread = true;
		threads[0].high = threads[0].offset + balls_on_thread - 1;

		//for (int i = 1; i<threads_count; i++)
		//{
		//	threads[i].broadphase_event = CreateEvent(NULL, TRUE, false, NULL);
		//	threads[i].end_broadphase_event = CreateEvent(NULL, TRUE, true, NULL);
		//	threads[i].is_main_thread = false;
		//	threads[i].offset = i*balls_on_thread;
		//	threads[i].high = threads[i].offset + balls_on_thread - 1;
		//	_beginthread(BroadPhase, 0, &threads[i]);
		//}
	}

	render->Set.ClearColor(0, 0, 0);
	float f = powf(2, fracture_part);
	ortho_m = TMatrix<float, 4>::GetOrtho(TVec2(1, 1)*room_size*0.5*f , TVec2(1, 1)*room_size*1.1*f , -10, 10);
	ortho_m_inv = ortho_m.GetInverted();

	render->Set.Projection(ortho_m);
	render->Set.Color(1, 1, 1, 1);
	//render->Blend.Enable(true);
	//render->Blend.Func("dc*(1-sa)+sc*sa");

	//render->Set.PointSize(point_size);
	//render->Set.PointSmooth(true);

	//sprintf_s(render->log_buff, " passed\n"); render->log_file.Write(render->log_buff);

	//tex = render->Texture.Create("frame_3.png");

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

	//sprintf_s(render->log_buff, "VBuffers creation..."); render->log_file.Write(render->log_buff);
	pos_buff = render->VertexBuffer.Create(TVBType::Array, balls_count*sizeof(TVec<short, 2>), TVBRefresh::Stream, TVBUsage::Draw);
	TVec<short, 2>* points = (TVec<short, 2>*)render->VertexBuffer.Map(pos_buff, TVBAccess::Write);
	SendBallsPos(points);
	render->VertexBuffer.Unmap(pos_buff);

#ifdef USE_COLOR
	color_buff = render->VertexBuffer.Create(TVBType::Array, balls_count*sizeof(TVec<unsigned char, 4>), TVBRefresh::Static, TVBUsage::Draw);
	TVec<unsigned char, 4>* colors = (TVec<unsigned char, 4>*)render->VertexBuffer.Map(color_buff, TVBAccess::Write);

	for (int i = 0; i<balls_count; i++)
	{
		balls_color[i] = TVec<unsigned char, 4>(
			255 * sinf(balls_pos[i][1] * TFloat(70.0f / room_size)),
			int(balls_pos[i][0] * TFloat(255.0f / room_size)),
			int(balls_pos[i][1] * TFloat(255.0f / room_size)),
			255);
		colors[i] = balls_color[i];
	}
	render->VertexBuffer.Unmap(color_buff);
#endif
	//sprintf_s(render->log_buff, " passed\n"); render->log_file.Write(render->log_buff);


#ifdef USE_COLOR
	{
		TVec<unsigned char, 4>* colors = (TVec<unsigned char, 4>*)render->VertexBuffer.Map(color_buff, TVBAccess::Write);
		memcpy(colors, &balls_color, sizeof(balls_color));
		render->VertexBuffer.Unmap(color_buff);
	}

	streams.AddStream(TStream::Color, TDataType::UByte, 4, color_buff);
#endif

	render->VertexBuffer.Data(pos_buff, balls_count*sizeof(TVec<TFloat, 2>), &balls_pos[0]);
	streams.AddStream(TStream::Vertex, TDataType::Int, 2, pos_buff);
}

static void draw_scene(GLFWwindow* window, double tt)
{

	//if (balu_time.GetDelta()<0.001)return;
	//balu_time.Tick();

	//action = KeyDown(VK_LBUTTON) ? 1 : (KeyDown(VK_RBUTTON) ? -1 : 0);
	//{
	//	TVec2i temp = GetCursorPos();
	//	TVec2 tt = render->ScreenToClipSpace(temp[0], temp[1]);
	//	TVec3 mm = (ortho_m_inv*(TVec4(TVec3(tt, 0), 1))).GetHomogen();
	//	float f = powf(2, fracture_part);
	//	mouse_world_pos[0] = mm[0] / f;
	//	mouse_world_pos[1] = mm[1] / f;
	//}
	//if (!balls_pause)
	//{
		//UpdateBalls(balu_time, !test_broadphase);
	//}

	auto t = balu_time.GetTime();
	//balu_time.Tick();
	if (balu_time.ShowFPS())
	{
		char buf[1000];
		sprintf_s(buf, "HumMan Balls: %d %7.1f FPS Frame: %.3f ms  Phys: %.3f  Draw: %.3f ",
			balls_count,
			balu_time.GetFPS(),
			balu_time.GetTick() * 1000,
			phys_time * 1000,
			draw_time * 1000);
		glfwSetWindowTitle(window, &buf[0]);
	}

	{
		render->Clear(1, 1);		
		{
			while (mtx_trylock(&thread_sync.particles_lock) != thrd_success);

			render->VertexBuffer.Data(pos_buff, balls_count*sizeof(TVec2_Float), &balls_pos[0]);
			render->VertexBuffer.Data(color_buff, balls_count*sizeof(TVec<unsigned char, 4>), &balls_color[0]); //цвета тоже загружаем повторно, т.к. шары мен€ют своЄ положение в массиве в цел€х оптимизации
			//TVec<short, 2>* points = (TVec<short, 2>*)render->VertexBuffer.Map(pos_buff, TVBAccess::Write);
			//SendBallsPos(points);
			//render->VertexBuffer.Unmap(pos_buff);

			
			//render->Blend.Func("dc*(1-sa)+sc*sa");
			//render->Set.Projection(ortho_m);

			// Wait for particle physics thread to be done
			
			//mtx_lock(&thread_sync.particles_lock);
			//while (!glfwWindowShouldClose(window) &&
			//	thread_sync.p_frame <= thread_sync.d_frame)
			{
				struct timespec ts;
				clock_gettime(CLOCK_REALTIME, &ts);
				ts.tv_nsec += 100000000;
				//cnd_timedwait(&thread_sync.p_done, &thread_sync.particles_lock, &ts);
			}

			render->Draw(streams, TPrimitive::Points, balls_count);
			//streams.Clear();

			// We are done with the particle data
			mtx_unlock(&thread_sync.particles_lock);
			//cnd_signal(&thread_sync.d_done);
		}

		if (false){
			//render->Blend.Func("dc*sa+sc*(1-sa)");
			TQuad<float, 3> q;
			TQuad<float, 2> t;
			t.Set(TVec2(0.5, 0.5), 1);
			q.Set(TVec2(0, 0), TVec2(1, 1), 0);
			streams.AddStream(TStream::Vertex, TDataType::Float, 3, &q);
			streams.AddStream(TStream::TexCoord, 0, TDataType::Float, 2, &t);
			render->Texture.Enable(true);
			//render->Texture.Bind(tex);
			render->Set.Color(1, 1, 1, 1);
			render->Set.Projection(TMatrix<float, 4>::GetIdentity());
			render->Draw(streams, TPrimitive::Quads, 4);
			render->Texture.Enable(false);
			streams.Clear();
		}

	}

	draw_time = balu_time.TimeDiff(balu_time.GetTime(), t);
}

static void resize_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	aspect_ratio = height ? width / (float)height : 1.f;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, 1);
			break;
		case GLFW_KEY_P:
			balls_pause = !balls_pause;
			break;
		case GLFW_KEY_B:
			test_broadphase = !test_broadphase;
			break;
		default:
			break;
		}
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	cursor_pos = TVec2(xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	attractor_size += (yoffset)*0.003;
	Clamp(0.0f, room_size / 2.0f, attractor_size);
	attractor_size = Clamp(0.01f, 1000.0f, attractor_size);
}

static int physics_thread_main(void* arg)
{
	GLFWwindow* window = (GLFWwindow*)arg;

	for (;;)
	{
		//if (balu_time.GetDelta()<0.001)return 0;
		

		while (mtx_trylock(&thread_sync.particles_lock) != thrd_success);

		// Wait for particle drawing to be done
		//while (!glfwWindowShouldClose(window) &&
		//	thread_sync.p_frame > thread_sync.d_frame)
		{
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec += 100000000;
			//cnd_timedwait(&thread_sync.d_done, &thread_sync.particles_lock, &ts);
		}

		if (glfwWindowShouldClose(window))
			break;

		balu_time.Tick();

		if (!balls_pause)
		{
			UpdateBalls(balu_time, !test_broadphase);
		}

		// Unlock mutex and signal drawing thread
		mtx_unlock(&thread_sync.particles_lock);
		//cnd_signal(&thread_sync.p_done);
	}

	return 0;
}

int main(int argc, char** argv)
{
	int ch, width, height;
	thrd_t physics_thread = 0;
	GLFWwindow* window;

	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	width = 640;
	height = 480;

	window = glfwCreateWindow(width, height, "Particle Engine", NULL, NULL);
	if (!window)
	{
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Set initial aspect ratio
	glfwGetFramebufferSize(window, &width, &height);
	resize_callback(window, width, height);

	render = new TBaluRender(TVec2i(width, height));

	Init();

	balu_time.Start();

	// Set initial times
	thread_sync.t = 0.0;
	thread_sync.dt = 0.001f;

	mtx_init(&thread_sync.particles_lock, mtx_timed);
	cnd_init(&thread_sync.p_done);
	cnd_init(&thread_sync.d_done);glfwSetTime(0.0);

	if (thrd_create(&physics_thread, physics_thread_main, window) != thrd_success)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetTime(0.0);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene(window, glfwGetTime());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	thrd_join(physics_thread, NULL);

	delete render;

	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
