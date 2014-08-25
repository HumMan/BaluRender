
#include "baluRender.h"
#include "windows.h"
#include <memory>

#include <GL\GL.h>

TBaluRender* render;
TTime time;
TFPSCamera* cam;
TVec2 dsf;
TBitmapFontId font;
TMatrix4 perspective_matrix;

std::vector<std::unique_ptr<TBVolume<float,3>>> volumes;

TVertexBufferId prim_vertex,prim_index;
int prim_index_count;

TVertexBufferId prim_lines_vertex;
int prim_lines_vertex_count;

float rz,ry;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam)
{
	TFPSCamera cam(TVec3(1,1,1),TVec3(1,1,1),TVec3(0,0,1));
	switch (message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		if(render)
			render->Set.Viewport(TVec2i(LOWORD(lParam),HIWORD(lParam)));
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
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

void Init()
{
	for(int i=0;i<1;i++)
		for(int k=0;k<1;k++)
			for(int t=0;t<1;t++)
				volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TAABB<float, 3>(TVec3(i * 4, k * 4, t * 4), TVec3(1, 1, 1))));

	for(int i=0;i<1;i++)
		for(int k=0;k<1;k++)
			for(int t=0;t<1;t++)
				volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TSphere<float, 3>(TVec3(i * 4, k * 4, t * 4 + 6), 1)));

	for(int i=0;i<1;i++)
		for(int k=0;k<1;k++)
			for(int t=0;t<1;t++)
			{
				TVec3 v0,v1;
				v0.MakeRandom();
				v1.MakeRandom();
				v0.Normalize();
				v1.Normalize();
				v0=v0.Cross(v1);
				v0.Normalize();
				volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TOBB<float, 3>(TVec3(i * 8, k * 8, t * 8 + 15), TMatrix3(v0, v1, v0.Cross(v1).GetNormalized()), TAABB<float, 3>(TVec3(0), TVec3(1)))));
			}

			for(int i=0;i<5;i++)
				for(int k=0;k<1;k++)
					for(int t=0;t<1;t++)
					{
						TVec3 v0,v1;
						v0=TVec3(i*8,k*8,t*8+22);
						v1.MakeRandom();
						v1=v0+v1*3;
						volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TCapsule<float, 3>(v0, v1, 2)));
						//volumes.Push(new TCapsule<float,3>(TVec3(5,0,-8),TVec3(8,0,-5),2));
					}

			rz=0;
			ry=0;

			render->Set.VSync(true);
			render->Depth.Test(true);
			ShowCursor(false);

			render->Set.ClearColor(0,0,0);
			render->Set.Color(1,1,1,1);
			render->Blend.Func("dc+sc*sa");
			cam= new TFPSCamera(TVec3(0,0,110),TVec3(0,0,-1),TVec3(0,1,0));
			cam->Activate(100,100);
			TVec2i screen_size=render->ScreenSize();
			perspective_matrix=TMatrix4::GetPerspective(90,screen_size[0],screen_size[1],0.01,1000);
			render->Set.Projection(perspective_matrix);

			font=render->BitmapFont.Create();

			{
				std::vector<TVec3> vertices;
				std::vector<unsigned int> indices;
				for(int i=0;i<volumes.size();i++)
				{
					volumes[i]->DrawTriangles(vertices,indices);
				}
				prim_index_count=indices.size();

				prim_vertex = render->VertexBuffer.Create(TVBType::Array, vertices.size()*sizeof(TVec3));
				prim_index = render->VertexBuffer.Create(TVBType::Index, indices.size()*sizeof(unsigned int));
				render->VertexBuffer.SubData(prim_vertex, 0, vertices.size()*sizeof(TVec3), &vertices[0]);
				render->VertexBuffer.SubData(prim_index, 0, indices.size()*sizeof(unsigned int), &indices[0]);
			}
			{
				std::vector<TVec3> vertices;
				for(int i=0;i<volumes.size();i++)
				{
					volumes[i]->DrawLines(vertices);
				}
				prim_lines_vertex_count = vertices.size();

				if(prim_lines_vertex_count>0)
				{

					prim_lines_vertex=render->VertexBuffer.Create(TVBType::Array,prim_lines_vertex_count*sizeof(TVec3));
					render->VertexBuffer.SubData(prim_lines_vertex,0,prim_lines_vertex_count*sizeof(TVec3),&vertices[0]);
				}
			}
}

void DrawVolumes()
{
	TStreamsDesc streams;

	streams.AddStream(TStream::Vertex,TDataType::Float,3,prim_vertex);
	streams.AddStream(TStream::Index,TDataType::UInt,1,prim_index);
	render->Draw(streams,TPrimitive::Triangles,prim_index_count);
	streams.Clear();
}

void DrawVolumesLines()
{
	TStreamsDesc streams;

	streams.AddStream(TStream::Vertex,TDataType::Float,3,prim_lines_vertex);
	render->Draw(streams,TPrimitive::Lines,prim_lines_vertex_count);
	streams.Clear();
}

void MainLoop()
{	
	if(time.GetDelta()<0.001)return;
	time.Tick();

	if(time.ShowFPS())
	{
		char buf[1000];
		sprintf_s(buf,"1 - Nearest 2 - Billenear    %7.1f FPS",time.GetFPS());
		SetWindowText(hWnd,buf);
	}

	cam->Update(time.GetTick());
	CheckGLError();

	render->BeginScene();
	{
		render->Clear(1,1);
		render->Set.Projection(perspective_matrix);
		render->Set.ModelView(cam->GetView());

		TMatrix<float,4> inv_mvp;
		inv_mvp=perspective_matrix*cam->GetView();
		inv_mvp.Invert();

		{
			render->Blend.Enable(true);

			render->Set.Color(1,1,0.7);
			DrawVolumesLines();
			render->Set.Color(0.3,0.9,0.2,0.7);
			//DrawVolumes();

			if(true)
			{
				float size=0.9;
				float step=0.01;
				//render->Set.PointSize(1);
				//render->Set.PointSmooth(true);
				for(float i=-size;i<=size;i+=step)
				{
					for(float j=-size;j<=size;j+=step)
					{
						TVec4 v0(i,j,0,1);
						TVec4 v1(i,j,1,1);
						v0=inv_mvp*v0;
						v1=inv_mvp*v1;
						TRay<float,3> ray;
						ray.pos=v0.GetHomogen();
						ray.dir=v1.GetHomogen()-ray.pos;
						ray.dir.Normalize();

						render->Set.Color(0,0,1,1);
						for (int k = 0; k < volumes.size(); k++)
						{
							float t,t0,t1,t2,t3;
							TVec3 n,n0,n1;
							bool c0 = volumes[k]->CollideWith(ray);
							bool c1 = volumes[k]->CollideWith(ray, t, n);
							bool c2 = volumes[k]->CollideWith(ray, t0, n0, t1, n1);
							bool c3 = volumes[k]->CollideWith(ray, t2, t3);
						
							if(c0&&c1&&c2&&c3)
							{
								//render->Set.Color(0,n*TVec3(0.5,1,1).GetNormalized(),0,1);
								render->Set.Color(0,1,0,1);
								break;
							}else if((!c0)&&(!c1)&&(!c2)&&(!c3))
							{
								render->Set.Color(0,0.2,0.2,1);
							}else
							{
								render->Set.Color(1,0,0,1);
								break;
							}
						}

						glBegin(GL_POINTS);
						glVertex3fv((GLfloat*)&(ray.pos+ray.dir));
						glEnd();
					}
				}
				//render->Set.PointSmooth(false);
				//render->Set.PointSize(1);
			}

			render->Blend.Enable(false);

		}
		{
			TStreamsDesc streams;
			TLine<float,3> line[3];
			TLine<unsigned char,4> color[3];
			line[0][0]=TVec3(0,0,0);
			line[0][1]=TVec3(1,0,0);
			color[0].Set(TVec<unsigned char,4>(255,0,0,255));
			line[1][0]=TVec3(0,0,0);
			line[1][1]=TVec3(0,1,0);
			color[1].Set(TVec<unsigned char,4>(0,255,0,255));
			line[2][0]=TVec3(0,0,0);
			line[2][1]=TVec3(0,0,1);
			color[2].Set(TVec<unsigned char,4>(0,0,255,255));
			streams.AddStream(TStream::Vertex,TDataType::Float, 3,&line);
			streams.AddStream(TStream::Color,TDataType::UByte, 4,&color);
			render->Draw(streams,TPrimitive::Lines,6);
			streams.Clear();

			render->Set.Color(1,1,1);

		}

	}render->EndScene();
}

int WINAPI WinMain (HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpCmdLine,
					int iCmdShow)
{   
	MSG msg;
	/* register window class */
	WNDCLASSA wc = {0};
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
	RegisterClassA (&wc);

	/* create main window */
	hWnd = CreateWindowA (
		wc.lpszClassName, "Sample",
		WS_OVERLAPPEDWINDOW| WS_VISIBLE|WS_CLIPCHILDREN | WS_CLIPSIBLINGS, /*| WS_POPUPWINDOW | WS_VISIBLE,*/
		30,30,1400,1000,//GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
		NULL, NULL, hInstance, NULL);

	RECT rect;
	GetClientRect(hWnd,&rect);

	render=new TBaluRender(hWnd,TVec2i(rect.right-rect.left,rect.bottom-rect.top));

	Init();

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

	delete render;

	/* destroy the window explicitly */
	DestroyWindow(hWnd);

	return 0;
}