
#include "baluRender.h"

#include <process.h>
#include "balls.h"

TBaluRender* render;

bool pause=false;
bool visualize=false;
bool test_broadphase=false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_MOUSEWHEEL:
		{
			attractor_size += (GET_WHEEL_DELTA_WPARAM(wParam))*0.003;
			attractor_size=Clamp(0.01f,1000.0f,attractor_size);
		}
		break;
	case WM_SIZE:
		if(render)
			render->Set.Viewport(LOWORD(lParam),HIWORD(lParam));
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'P':
			pause=!pause;
			break;
		case 'V':
			visualize=!visualize;
			break;
		case 'B':
			test_broadphase=!test_broadphase;
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

char buf[1000];

//GLuint	base;	

TVector<TVec2> points;
TVector<TVec<unsigned char,4>> colors;
//TVector<TBaluLine> pairs;

//TBaluTexture tex1,tex2,rt;

TTime time;
double draw_time;

//void BuildFont()					// Build Our Bitmap Font
//{
//	HFONT	font;						// Windows Font ID
//	base = glGenLists(256);					// Storage For 256 Characters		
//	font = CreateFont(	-15,				// Height Of Font		
//				0,						
//				0,				
//				0,						
//				FW_BOLD,			
//				FALSE,				
//				FALSE,				
//				FALSE,					
//				ANSI_CHARSET,				
//				OUT_TT_PRECIS,				
//				CLIP_DEFAULT_PRECIS,				
//				ANTIALIASED_QUALITY,			
//				FF_DONTCARE|DEFAULT_PITCH,	
//				"Comic Sans MS");			
//	SelectObject(render->GetBaluRenderDC(), font);				
//	bool b=wglUseFontBitmaps(render->GetBaluRenderDC(),0, 256, base);
//}

void MainLoop()
{	
	if(time.GetDelta()<0.001)return;
	time.Tick();

	action=KeyDown(VK_LBUTTON)?1:(KeyDown(VK_RBUTTON)?-1:0);
	mouse_world_pos=render->GetMouseWorldPos();
	if(!pause)
	{
		float step=time.GetTick();
		if(step>0.012)
			step=0.012;
		UpdateBalls(0.005/*step*/,time,!test_broadphase);
	}

	UINT64 t=time.GetTime();

	if(time.ShowFPS())
	{
		sprintf(&buf[0],"Balls: %d %7.1f FPS Frame: %.3f ms  Phys: %.3f  Draw: %.3f ",
			balls_count,
			time.GetFPS(),
			time.GetTick()*1000,
			phys_time*1000,
			draw_time*1000);
		SetWindowText(hWnd,&buf[0]);
	}

	CheckGlError();

	GetDrawInfo(points);

	render->BeginScene();{
		render->Clear(1,1);

		//render->Textures.Enable(true);
		render->Set.Color(1,1,1,1);

		//TBaluQuad fscr;
		//TBaluTexCoords fscr_coords;
		//fscr.Init(TVec2(1,1)*room_size*0.5,TVec2(1,1)*room_size,0,0);
		//fscr_coords.Init(TVec2(0.5,0.5),5.0);
		//render->Textures.Bind(tex2);
		//render->Quads.Begin();
		//render->Quads.Draw(fscr,fscr_coords);
		//render->Quads.End();

		//render->Textures.Enable(false);

		//render->AlphaTest.Enable(true);
		render->Blend.Enable(true);
		//render->Textures.Bind(tex1);
		//render->AlphaTest.SetFunc(">=",0.1);
		render->Blend.SetFunc("dc*(1-sa)+sc*sa");
		//render->Quads.DrawBatch(quads,quads_texcoords,colors);

		render->Set.PointsSize(3);
		render->Set.PointSmooth(true);
		render->Points.DrawBatch(points,colors);

		//визуализация grid
		/*if(visualize)
		{
			render->Blend.Enable(true);
			render->Textures.Enable(false);
			render->AlphaTest.Enable(false);

			render->Quads.Begin();
			for(int i=0;i<blocks_count;i++)
				for(int k=0;k<blocks_count;k++)
				{
					if(grid_count[i*blocks_count+k]==0)continue;
					render->Set.Color(1,1,1,0.15*grid_count[i*blocks_count+k]);
					TBaluQuad b;
					b.Init((TVec2(k,i)+TVec2(0.5,0.5))*block_size,TVec2(1,1)*block_size,0,0);
					render->Quads.Draw(b);
				}
			render->Quads.End();

			render->Set.Color(1,1,1,1);

			render->Lines.Begin();
			{
				for(int i=0;i<balls_count;i++)
				{
					int cell_x=(balls_pos[i][0]);
					int cell_y=(balls_pos[i][1]);
					{
						for(int t=cell_x-1;t<=cell_x+1;t++)
							for(int k=cell_y-1;k<=cell_y+1;k++)
							{
								if(k>=0&&k<blocks_count&&t>=0&&t<blocks_count)
								{
									int cell_id=(k*blocks_count+t);
									for(int c=0;c<grid_count[cell_id];c++)
										{
											TVec2_base<TFloat> p1(balls_pos[i]),
												p2(balls_pos[grid[cell_id*MAX_BALLS_IN_BLOCK+c]]);
											TBaluLine l;
											l.Init(TVec2(p1[0],p1[1]),TVec2(p2[0],p2[1]),0);
											render->Lines.Draw(l);
										}
								}
							}
					}
				}
			}
			render->Lines.End();

			render->Textures.Enable(true);
			render->AlphaTest.Enable(true);
			render->Blend.Enable(false);
			render->Set.Color(1,1,1,1);
		}*/

		/*render->AlphaTest.Enable(false);
		render->Blend.Enable(false);
		render->Textures.Enable(false);*/

		// Display a string
		render->Set.Color(1,0.8,0.4,1);

		//glListBase(base); 
		//char a[1000];
		//sprintf(a,"Press 'P' - to pause; 'V' - to visualize; 'B' - only broadphase; ScrollUp/Down - change size of Attractor=%f",attractor_size);
		//glWindowPos2iARB(3, 5);
		//glCallLists(strlen(&a[0]), GL_UNSIGNED_BYTE, &a);

		//if(visualize)
		//{
		//	int ball_id=-1;
		//	TVec2 m=render->GetMouseWorldPos();
		//	int b_id=PosQuery(m);
		//	if(b_id>-1)
		//	{
		//		int x,y;
		//		render->GetPixelPos(m,x,y);
		//		render->Set.Color(1,0.8,0.4,1);
		//		glWindowPos2iARB(x, y);
		//		glListBase(base); 
		//		char a[100];
		//		sprintf(&a[0],"id = %i",b_id);
		//		glCallLists(strlen(&a[0]), GL_UNSIGNED_BYTE, &a);
		//	}
		//}

	}render->EndScene();

	draw_time=time.TimeDiff(time.GetTime(),t);
}

template <class T>
struct TVectorOp
{
	template <int count>
	static __forceinline T Sum(T* vec)
	{
		return vec[count]+(count==0?0:Sum<count-1>(vec));
	}
	template <>
	static __forceinline T Sum<0>(T* vec)
	{
		return 0;
	}
};

int WINAPI WinMain (HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int iCmdShow)
{    

	int v[8];

	int res=TVectorOp<int>::Sum<30>(&v[0]);

	printf("%i",res);

	MSG msg;
	/* register window class */
	WNDCLASS wc = {0};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Sample";
	RegisterClass (&wc);

	/* create main window */
	hWnd = CreateWindow (
		wc.lpszClassName, "Sample",
		WS_OVERLAPPEDWINDOW| WS_VISIBLE|WS_CLIPCHILDREN | WS_CLIPSIBLINGS, /*| WS_POPUPWINDOW | WS_VISIBLE,*/
		50, 30, 900,900,/*0,0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),*/
		NULL, NULL, hInstance, NULL);

	RECT rect;
	GetClientRect(hWnd,&rect);

	render=new TBaluRender(hWnd,rect.right-rect.left,rect.bottom-rect.top);

	render->Set.VSync(false);

	BuildFont();	

	//tex1=render->Textures.Create("textures/ball.png");
	//tex2=render->Textures.Create("textures/met_p_dorside.bmp");
	//rt=render->Textures.CreateTarget();

	render->Set.ClearColor(0,0,0);
	render->Set.Camera(TVec2(1,1)*room_size*0.5,TVec2(1,1)*room_size*1.1);

	//time.Start();

	quads.SetCount(balls_count);
	points.SetCount(balls_count);
	quads_texcoords.SetCount(balls_count);
	colors.SetCount(balls_count);


	for(int i=0;i<balls_count;i++)
	{
		int balls_on_line=room_size/(ball_rad*2+0.01);
		balls_pos[i]=(
			TVec2_base<TFloat>(
			i%balls_on_line,
			i/balls_on_line)
			+
			TVec2_base<TFloat>(0.5f,0.5f))*
			(2.0f*ball_rad+Randf()*0.0001f);

		quads_texcoords[i].Init(TVec2(0.5,0.5),1,0);
		colors[i].Init(sinf(balls_pos[i][1]),balls_pos[i][0]/TFloat(room_size),balls_pos[i][1]/TFloat(room_size));
		quads[i].Init(TVec2(balls_pos[i][0],balls_pos[i][1]),TVec2(ball_rad,ball_rad)*2,0,0);
	}

	time.Start();

	/* program main loop */
	while (true)
	{
		/* check for messages */
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
		{
			/* handle or dispatch messages */
			if (msg.message == WM_QUIT)
				break;
			else
			{
				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		}
		else
			MainLoop();
	}

	/* destroy the window explicitly */
	DestroyWindow(hWnd);

	delete render;

	return 0;
}