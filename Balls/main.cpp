#include "baluRender.h"
#include "windows.h"

#include "balls.h"

#include <process.h>  

TBaluRender* render;

bool pause = false;
bool test_broadphase = false;

using namespace TBaluRenderEnums;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_MOUSEWHEEL:
	{
		attractor_size += (GET_WHEEL_DELTA_WPARAM(wParam))*0.003;
		Clamp(0.0f, room_size / 2.0f, attractor_size);
		attractor_size = Clamp(0.01f, 1000.0f, attractor_size);
	}
		break;
	case WM_SIZE:
		if (render)
			render->Set.Viewport(TVec2i(LOWORD(lParam), HIWORD(lParam)));
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'P':
			pause = !pause;
			break;
		case 'B':
			test_broadphase = !test_broadphase;
			break;
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND hWnd;

TVertexBufferId pos_buff;

#ifdef USE_COLOR
TVertexBufferId color_buff;
#endif

TTextureId tex;

TMatrix<float, 4> ortho_m, ortho_m_inv;

TTime time;
double draw_time;

TStreamsDesc streams;

void Init()
{

	sprintf_s(render->log_buff, "Creating threads..."); render->log_file.Write(render->log_buff);
	{
		threads[0].offset = 0;
		threads[0].is_main_thread = true;
		threads[0].high = threads[0].offset + balls_on_thread - 1;

		for (int i = 1; i<threads_count; i++)
		{
			threads[i].broadphase_event = CreateEvent(NULL, TRUE, false, NULL);
			threads[i].end_broadphase_event = CreateEvent(NULL, TRUE, true, NULL);
			threads[i].is_main_thread = false;
			threads[i].offset = i*balls_on_thread;
			threads[i].high = threads[i].offset + balls_on_thread - 1;
			_beginthread(BroadPhase, 0, &threads[i]);
		}
	}
	sprintf_s(render->log_buff, " passed\n"); render->log_file.Write(render->log_buff);

	sprintf_s(render->log_buff, "Render setup..."); render->log_file.Write(render->log_buff);
	render->Set.VSync(false);

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

	sprintf_s(render->log_buff, " passed\n"); render->log_file.Write(render->log_buff);

	tex = render->Texture.Create("frame_3.png");

	for (int i = 0; i<balls_count; i++)
	{
		int balls_on_line = room_size / (ball_rad * 2 + 0.01);
		if (false)
		{
			balls_pos[i] = (
				TVec2_Float(
				short int(i%balls_on_line),
				short int(i / balls_on_line)) + TVec2_Float(0.5f, 0.5f)*(1 + Randfs()*0.001f));
		}
		else
		{
			balls_pos[i] = TVec2_Float(0.5f + Randf()*(room_size - 1), 0.5f + Randf()*(room_size - 1));
		}
	}

	sprintf_s(render->log_buff, "VBuffers creation..."); render->log_file.Write(render->log_buff);
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
	sprintf_s(render->log_buff, " passed\n"); render->log_file.Write(render->log_buff);


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

void MainLoop()
{
	if (time.GetDelta()<0.001)return;
	time.Tick();

	action = KeyDown(TBaluVirtKey::LButton) ? 1 : (KeyDown(TBaluVirtKey::RButton) ? -1 : 0);
	{
		TVec2i temp = GetCursorPos();
		TVec2 tt = render->ScreenToClipSpace(temp[0], temp[1]);
		TVec3 mm = (ortho_m_inv*(TVec4(TVec3(tt, 0), 1))).GetHomogen();
		float f = powf(2, fracture_part);
		mouse_world_pos[0] = mm[0] / f;
		mouse_world_pos[1] = mm[1] / f;
	}
	if (!pause)
	{
		UpdateBalls(time, !test_broadphase);
	}

	UINT64 t = time.GetTime();

	if (time.ShowFPS())
	{
		char buf[1000];
		sprintf_s(buf, "HumMan Balls: %d %7.1f FPS Frame: %.3f ms  Phys: %.3f  Draw: %.3f ",
			balls_count,
			time.GetFPS(),
			time.GetTick() * 1000,
			phys_time * 1000,
			draw_time * 1000);
		SetWindowText(hWnd, &buf[0]);
	}

	render->BeginScene();
	{
		render->Clear(1, 1);		
		{
			render->VertexBuffer.Data(pos_buff, balls_count*sizeof(TVec2_Float), &balls_pos[0]);
			render->VertexBuffer.Data(color_buff, balls_count*sizeof(TVec<unsigned char, 4>), &balls_color[0]); //цвета тоже загружаем повторно, т.к. шары мен€ют своЄ положение в массиве в цел€х оптимизации
			//TVec<short, 2>* points = (TVec<short, 2>*)render->VertexBuffer.Map(pos_buff, TVBAccess::Write);
			//SendBallsPos(points);
			//render->VertexBuffer.Unmap(pos_buff);

			
			//render->Blend.Func("dc*(1-sa)+sc*sa");
			//render->Set.Projection(ortho_m);
			render->Draw(streams, TPrimitive::Points, balls_count);
			//streams.Clear();
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
			render->Texture.Bind(tex);
			render->Set.Color(1, 1, 1, 1);
			render->Set.Projection(TMatrix<float, 4>::GetIdentity());
			render->Draw(streams, TPrimitive::Quads, 4);
			render->Texture.Enable(false);
			streams.Clear();
		}

	}render->EndScene();

	draw_time = time.TimeDiff(time.GetTime(), t);
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int iCmdShow)
{
	MSG msg;
	/* register window class */
	WNDCLASS wc = { 0 };
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Sample";
	RegisterClass(&wc);

	/* create main window */
	hWnd = CreateWindow(
		wc.lpszClassName, "Sample",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, /*| WS_POPUPWINDOW | WS_VISIBLE,*/
		30, 30, 1000, 1000,/*0,0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),*/
		NULL, NULL, hInstance, NULL);

	RECT rect;
	GetClientRect(hWnd, &rect);

	//sprintf_s(render->log_buff, "Render creation..."); render->log_file.Write(render->log_buff);

	render = new TBaluRender((int)hWnd, TVec2i(rect.right - rect.left, rect.bottom - rect.top));

	Init();

	time.Start();

	/* program main loop */
	while (true)
	{
		/* check for messages */
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			/* handle or dispatch messages */
			if (msg.message == WM_QUIT)
				break;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
			MainLoop();
	}

	delete render;

	/* destroy the window explicitly */
	DestroyWindow(hWnd);

	return 0;
}