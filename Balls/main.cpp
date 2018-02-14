
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <atomic>

#include <GLFW/glfw3.h>
#include "baluRender.h"
#include "balls.h"

std::atomic_flag balls_draw_lock = ATOMIC_FLAG_INIT;

#if defined(WIN32)||defined(_WIN32)
#else
#define sprintf_s sprintf
#endif

using namespace BaluRender;
TBaluRender* render;

TVec2 cursor_pos;
bool balls_pause = false;
bool test_broadphase_pressed = false;

TTime draw_fps, phys_fps;

using namespace TBaluRenderEnums;
TVertexBufferId pos_buff;
#ifdef USE_COLOR
TVertexBufferId color_buff;
#endif
TMatrix<float, 4> ortho_m, ortho_m_inv;

TVec2_Float local_balls_pos[balls_count];
TVec<unsigned char, 4> local_balls_color[balls_count];

TStreamsDesc streams;

int phys_fps_value;
double phys_time;

int curr_threads_count = 1;

bool volatile should_close = false;
bool volatile draw_waiting = false;

TTexFontId font;

void Init()
{

	font = render->TexFont.Create("font.ttf",15);

	InitBalls(curr_threads_count);

	render->Set.ClearColor(0, 0, 0);
	float f = powf(2, fracture_part);
	ortho_m = TMatrix<float, 4>::GetOrtho(TVec2(1, 1)*room_size*0.5*f, TVec2(1, 1)*room_size*1.1*f, -10, 10);
	ortho_m_inv = ortho_m.GetInverted();
	render->Set.Projection(ortho_m);
	render->Set.Color(1, 1, 1, 1);

	pos_buff = render->VertexBuffer.Create(TVBType::Array, balls_count * sizeof(TVec<short, 2>), TVBRefresh::Stream, TVBUsage::Draw);
	TVec<short, 2>* points = (TVec<short, 2>*)render->VertexBuffer.Map(pos_buff, TVBAccess::Write);
	SendBallsPos(points);
	render->VertexBuffer.Unmap(pos_buff);

#ifdef USE_COLOR
	color_buff = render->VertexBuffer.Create(TVBType::Array, balls_count * sizeof(TVec<unsigned char, 4>), TVBRefresh::Static, TVBUsage::Draw);
	TVec<unsigned char, 4>* colors = (TVec<unsigned char, 4>*)render->VertexBuffer.Map(color_buff, TVBAccess::Write);
	for (int i = 0; i < balls_count; i++)
	{
		colors[i] = GetBallsColorArray()[i];
	}
	render->VertexBuffer.Unmap(color_buff);
#endif

#ifdef USE_COLOR
	{
		TVec<unsigned char, 4>* colors = (TVec<unsigned char, 4>*)render->VertexBuffer.Map(color_buff, TVBAccess::Write);
		memcpy(colors, GetBallsColorArray(), sizeof(TVec<unsigned char, 4>)* balls_count);
		render->VertexBuffer.Unmap(color_buff);
	}
	streams.AddStream(TStream::Color, TDataType::UByte, 4, color_buff);
#endif
	render->VertexBuffer.Data(pos_buff, balls_count * sizeof(TVec<TFloat, 2>), GetBallsPosArray());
	streams.AddStream(TStream::Vertex, TDataType::Int, 2, pos_buff);


}

void error_callback(int error, const char* description)
{
	puts(description);
}
char buf[1000] = "";
static bool draw_scene(GLFWwindow* window, double tt)
{
	static double t_old = 0.0;
	float dt;

	double t = glfwGetTime();

	dt = (float)(t - t_old);

	if (dt < 0.01)
		return false;
	else
		t_old = t;

	draw_fps.Tick();
	
	if (draw_fps.ShowFPS())
	{
		sprintf_s(buf, "Balls: %d Thr %d Draw FPS: %7.1f Frame: %.1f ms Phys FPS: %d Phys frame: %.1f ms",
			balls_count,
			curr_threads_count,
			draw_fps.GetFPS(),
			draw_fps.GetTick() * 1000,
			phys_fps_value,
			phys_time * 1000);
		glfwSetWindowTitle(window, &buf[0]);
		puts(buf);
	}
	{
		render->Set.Projection(ortho_m);
		render->Clear(1, 1);
		{
			draw_waiting = true;

			while (balls_draw_lock.test_and_set(std::memory_order_acquire));

			CopyPosAndColor(&local_balls_pos[0], &local_balls_color[0]);

			draw_waiting = false;

			balls_draw_lock.clear(std::memory_order_release);

			render->VertexBuffer.Data(pos_buff, balls_count * sizeof(TVec2_Float), &local_balls_pos[0]);
#ifdef USE_COLOR
			render->VertexBuffer.Data(color_buff, balls_count * sizeof(TVec<unsigned char, 4>), &local_balls_color[0]);
#endif

			render->Draw(streams, TPrimitive::Points, balls_count);

			auto screen_size = render->Get.Viewport();

			auto ortho2 = TMatrix<float, 4>::GetOrtho(0, screen_size[0],0, screen_size[1], -10, 10);
			render->Set.Projection(ortho2);
			render->Blend.Enable(true);
			render->Blend.Func("dc*(1-sa)+sc*sa");
			
			render->TexFont.Print(font, TVec2(10, 10), buf);
		}
	}

	return true;
}
static void resize_callback(GLFWwindow* window, int width, int height)
{
	render->Set.Viewport(TVec2i(width, height));
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
			test_broadphase_pressed = !test_broadphase_pressed;
			break;
		case GLFW_KEY_MINUS:
			curr_threads_count = Clamp(1, 255, curr_threads_count - 1);
			ChangeThreadsCount(curr_threads_count);
			break;
		case GLFW_KEY_EQUAL:
			curr_threads_count = Clamp(1, 255, curr_threads_count + 1);
			ChangeThreadsCount(curr_threads_count);
			break;
		default:
			break;
		}
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	TVec2i temp = TVec2i(xpos, ypos);

	TVec2 tt = render->WindowToClipSpace(temp[0], temp[1]);
	TVec3 mm = (ortho_m_inv*(TVec4(TVec3(tt, 0), 1))).GetHomogen();
	float f = powf(2, fracture_part);
	SetAttractorPos(TVec2(mm[0] / f, mm[1] / f));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		SetAction(action == GLFW_PRESS ? 1 : 0);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		SetAction(action == GLFW_PRESS ? -1 : 0);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ChangeAttractor(yoffset);
}

static int physics_thread_main(void* arg)
{
	GLFWwindow* window = (GLFWwindow*)arg;
	for (;;)
	{
		if (should_close)
			break;

		if (draw_waiting)
			thrd_yield();

		while (balls_draw_lock.test_and_set(std::memory_order_acquire));

		if (!balls_pause)
		{
			UpdateBalls(test_broadphase_pressed);
			phys_fps.Tick();
		}

		balls_draw_lock.clear(std::memory_order_release);

		if (phys_fps.ShowFPS())
		{
			phys_fps_value = phys_fps.GetFPS();
			phys_time = phys_fps.GetTick();
		}
	}
	printf("phys cycle stopped\n");
	return 0;
}

int main(int argc, char** argv)
{
	glfwSetErrorCallback(error_callback);
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

	render = new TBaluRender(TVec2i(width, height));

	Init();

	resize_callback(window, width, height);

	
	draw_fps.Start();
	phys_fps.Start();

	glfwSetTime(0.0);
	if (thrd_create(&physics_thread, physics_thread_main, window) != thrd_success)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetTime(0.0);
	printf("Starting main cycle\n");
	while (!glfwWindowShouldClose(window))
	{
		if (draw_scene(window, glfwGetTime()))
		{
			glfwSwapBuffers(window);
		}
		glfwPollEvents();
	}
	should_close = true;
	printf("Exiting\n");
	thrd_join(physics_thread, NULL);
	delete render;
	printf("Render deleted\n");
	glfwDestroyWindow(window);
	glfwTerminate();
	printf("glfwTerminated\n");
	return 0;
}