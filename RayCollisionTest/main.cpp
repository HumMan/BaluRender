
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


using namespace BaluRender;

using namespace BaluLib;
using namespace TBaluRenderEnums;

TBaluRender* render;
TTime balu_time;
TFPSCamera* cam;
TVec2 dsf;

float aspect_ratio;

TVec2 cursor_pos;


TMatrix4 perspective_matrix;

std::vector<std::unique_ptr<TBVolume<float, 3>>> volumes;

TVertexBufferId prim_vertex, prim_index;
int prim_index_count;

TVertexBufferId prim_lines_vertex;
int prim_lines_vertex_count;

float rz, ry;

int viewport_width, viewport_height;

TVec<unsigned char, 4>* raytracer_color_buffer;

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

	//for(int i=0;i<1;i++)
	//	for(int k=0;k<1;k++)
	//		for(int t=0;t<1;t++)
	//		{
	//			TVec3 v0,v1;
	//			v0.MakeRandom();
	//			v1.MakeRandom();
	//			v0.Normalize();
	//			v1.Normalize();
	//			v0=v0.Cross(v1);
	//			v0.Normalize();
	//			volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TOBB<float, 3>(TVec3(i * 8, k * 8, t * 8 + 15), TMatrix3(v0, v1, v0.Cross(v1).GetNormalized()), TAABB<float, 3>(TVec3(0), TVec3(1)))));
	//		}

	for (int i = 0; i < 1; i++)
	{
		for (int k = 0; k < 1; k++)
		{
			for (int t = 0; t < 1; t++)
			{
				TVec3 v0, v1;
				v0 = TVec3(i * 8, k * 8, t * 8 + 22);
				v1.MakeRandom();
				v1 = v0 + v1 * 3;
				volumes.push_back(std::unique_ptr<TBVolume<float, 3>>(new TCapsule<float, 3>(v0, v1, 2)));
			}
		}
	}
	rz = 0;
	ry = 0;

	render->Depth.Test(true);
	ShowCursor(false);

	render->Set.ClearColor(0, 0, 0);
	render->Set.Color(1, 1, 1, 1);
	render->Blend.Func("dc+sc*sa");
	cam = new TFPSCamera(TVec3(0, 0, 110), TVec3(0, 0, 1), TVec3(0, 1, 0));
	cam->Activate(100, 100);
	TVec2i screen_size = render->ScreenSize();
	perspective_matrix = TMatrix4::GetPerspective(90, screen_size[0], screen_size[1], 0.01, 1000);
	render->Set.Projection(perspective_matrix);

	{
		std::vector<TVec3> vertices;
		std::vector<unsigned int> indices;
		for (int i = 0; i < volumes.size(); i++)
		{
			volumes[i]->DrawTriangles(vertices, indices);
		}
		prim_index_count = indices.size();

		prim_vertex = render->VertexBuffer.Create(TVBType::Array, vertices.size()*sizeof(TVec3));
		prim_index = render->VertexBuffer.Create(TVBType::Index, indices.size()*sizeof(unsigned int));
		render->VertexBuffer.SubData(prim_vertex, 0, vertices.size()*sizeof(TVec3), &vertices[0]);
		render->VertexBuffer.SubData(prim_index, 0, indices.size()*sizeof(unsigned int), &indices[0]);
	}
			{
				std::vector<TVec3> vertices;
				for (int i = 0; i < volumes.size(); i++)
				{
					volumes[i]->DrawLines(vertices);
				}
				prim_lines_vertex_count = vertices.size();

				if (prim_lines_vertex_count > 0)
				{

					prim_lines_vertex = render->VertexBuffer.Create(TVBType::Array, prim_lines_vertex_count*sizeof(TVec3));
					render->VertexBuffer.SubData(prim_lines_vertex, 0, prim_lines_vertex_count*sizeof(TVec3), &vertices[0]);
				}
			}

			raytracer_color_buffer = new TVec<unsigned char, 4>[viewport_width*viewport_height];
}

void DrawVolumes()
{
	TStreamsDesc streams;

	streams.AddStream(TStream::Vertex, TDataType::Float, 3, prim_vertex);
	streams.AddStream(TStream::Index, TDataType::UInt, 1, prim_index);
	render->Draw(streams, TPrimitive::Triangles, prim_index_count);
	streams.Clear();
}

void DrawVolumesLines()
{
	TStreamsDesc streams;

	streams.AddStream(TStream::Vertex, TDataType::Float, 3, prim_lines_vertex);
	render->Draw(streams, TPrimitive::Lines, prim_lines_vertex_count);
	streams.Clear();
}

static void draw_scene(GLFWwindow* window, double tt)
{
	balu_time.Tick();

	if (balu_time.ShowFPS())
	{
		char buf[1000];
		sprintf_s(buf, "1 - Nearest 2 - Billenear    %7.1f FPS", balu_time.GetFPS());
		glfwSetWindowTitle(window, buf);
	}

	cam->UpdateView();

	CheckGLError();

	{
		//render->Set.ClearColor(0, 0, 1);
		render->Clear(1, 1);
		render->Set.Projection(perspective_matrix);
		render->Set.ModelView(cam->GetView());

		TMatrix<float, 4> inv_mvp;
		inv_mvp = perspective_matrix*cam->GetView();
		inv_mvp.Invert();

		{
			render->Blend.Enable(false);

			render->Set.Color(1, 1, 0.7);
			//DrawVolumesLines();
			render->Set.Color(0.3, 0.9, 0.2, 0.7);
			//DrawVolumes();
		}
		//render->Clear(1, 1);
		render->Set.Projection(TMatrix<float, 4>::GetIdentity());
		render->Set.ModelView(TMatrix<float, 4>::GetIdentity());

		if (true)
		{
			float pixel_size_x = 2.0 / (viewport_width);
			float pixel_size_y = 2.0 / (viewport_height);
			//float size = 0.9;
			//float step = 0.002;
			//render->Set.PointSize(1);
			//render->Set.PointSmooth(true);

			//glBegin(GL_POINTS);

//#pragma omp parallel for
			for (int x = 1; x < viewport_width-1; x += 1)
			{
				//break;
				for (int y = 1 ; y < viewport_height-1; y += 1)
				{
					float i = x*pixel_size_x - 1;
					float j = y*pixel_size_y - 1;
					TVec4 v0(i, j, 0, 1);
					TVec4 v1(i, j, 1, 1);
					v0 = inv_mvp*v0;
					v1 = inv_mvp*v1;
					TRay<float, 3> ray;
					ray.pos = v0.GetHomogen();
					ray.dir = v1.GetHomogen() - ray.pos;
					ray.dir.Normalize();
					//ray.dir = -ray.dir;

					//render->Set.Color(0, 0, 1, 1);

					//TVec3 color(0, 0, 1);
					TVec<unsigned char, 4> color(0, 0, 1);
					color = TVec<unsigned char, 4>(0, 0.2 * 255, 0.2 * 255, 255);
					for (int k = 0; k < volumes.size(); k++)
					{
						//float t, t0, t1, t2, t3;
						//TVec3 n, n0, n1;
						//bool c0 = volumes[k]->CollideWith(ray);
						//bool c1 = volumes[k]->CollideWith(ray, t, n);
						//bool c2 = volumes[k]->CollideWith(ray, t0, n0, t1, n1);
						//bool c3 = volumes[k]->CollideWith(ray, t2, t3);

						TRayCollisionInfo<float, 3> info, info2;

						//bool c2 = volumes[k]->RayCollide(ray);
						//bool c1 = volumes[k]->RayCollide(ray, info);

						//bool c1 = volumes[k]->SegmentCollide(TSegment<float, 3>(ray.pos, ray.pos + ray.dir * 1000));
						//bool c1 = volumes[k]->SegmentCollide(TSegment<float, 3>(ray.pos + ray.dir * 40, ray.pos));
						//bool c1 = volumes[k]->SegmentCollide(TSegment<float, 3>(ray.pos, ray.pos + ray.dir * 30), info);

						//bool c1 = volumes[k]->LineCollide(TLine<float, 3>(ray.pos + ray.dir * 40, ray.pos));
						bool c1 = volumes[k]->LineCollide(TLine<float, 3>(ray.pos, ray.pos + ray.dir * 40), info);
						
						//assert(c0 == c1);

						//if (c2)
						//{
						//	color = TVec<unsigned char, 4>(1*255, 1*255, 0, 255);
						//}else
						//	color = TVec<unsigned char, 4>(0, 0.2*255, 0, 255);

						if (c1)
						{
							float col = 1;
							if (info.have_in)
								col = abs(Clamp<float>(0, 1, -info.in_normal*ray.dir));
							else if (info.have_out)
								col = abs(Clamp<float>(0, 1, info.out_normal*ray.dir));
							//else assert(false);
							color = TVec<unsigned char,4>(0, col*255, 0,255);
							//render->Set.Color(0, col, 0, 1);
						}
						else
						{
							//color = TVec<unsigned char, 4>(0, 0.2 * 255, 0.2 * 255,255);
							//render->Set.Color(0, 0.2, 0.2, 1);
							//break;
						}

						//if (c0&&c1&&c2&&c3)
						//{
						//	//render->Set.Color(0,n*TVec3(0.5,1,1).GetNormalized(),0,255);
						//	render->Set.Color(0, 1, 0, 1);
						//	break;
						//}
						//else if ((!c0) && (!c1) && (!c2) && (!c3))
						//{
						//	render->Set.Color(0, 0.2, 0.2, 1);
						//}
						//else
						//{
						//	render->Set.Color(1, 0, 0, 1);
						//	break;
						//}
					}

					
					//glVertex3fv((GLfloat*)&(ray.pos + ray.dir));
					//glColor3fv((GLfloat*)&color);
					//glVertex2fv((GLfloat*)&(TVec<float, 2>(i,j)));
					raytracer_color_buffer[y*viewport_width + x] = color;
					
				}
			}

			glRasterPos2d(-1, -1);
			
			glDrawPixels(viewport_width, viewport_height, GL_RGBA, GL_UNSIGNED_BYTE, (GLfloat*)&raytracer_color_buffer[0]);

			//glEnd();

			//render->Set.PointSmooth(false);
			//render->Set.PointSize(1);
		}

		render->Blend.Enable(false);

	}
	if (false)
		{
			TStreamsDesc streams;
			TGeomLine<float, 3> line[3];
			TGeomLine<unsigned char, 4> color[3];
			line[0][0] = TVec3(0, 0, 0);
			line[0][1] = TVec3(1, 0, 0);
			color[0].Set(TVec<unsigned char, 4>(255, 0, 0, 255));
			line[1][0] = TVec3(0, 0, 0);
			line[1][1] = TVec3(0, 1, 0);
			color[1].Set(TVec<unsigned char, 4>(0, 255, 0, 255));
			line[2][0] = TVec3(0, 0, 0);
			line[2][1] = TVec3(0, 0, 1);
			color[2].Set(TVec<unsigned char, 4>(0, 0, 255, 255));
			streams.AddStream(TStream::Vertex, TDataType::Float, 3, &line);
			streams.AddStream(TStream::Color, TDataType::UByte, 4, &color);
			render->Draw(streams, TPrimitive::Lines, 6);
			streams.Clear();

			render->Set.Color(1, 1, 1);

		}
}


static void resize_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	aspect_ratio = height ? width / (float)height : 1.f;
	viewport_width = width;
	viewport_height = height;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		TFPSCamera::Key bkey(TFPSCamera::Key::None);
		float s = mods && GLFW_MOD_SHIFT ? 3 : 1;
		switch (key)
		{
		case GLFW_KEY_A:
			bkey = TFPSCamera::Key::Left;
			break;
		case GLFW_KEY_D:
			bkey = TFPSCamera::Key::Right;
			break;
		case GLFW_KEY_S:
			bkey = TFPSCamera::Key::Down;
			break;
		case GLFW_KEY_W:
			bkey = TFPSCamera::Key::Up;
			break;

		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, 1);
			break;
		default:
			break;
		}

		cam->KeyDown(bkey, balu_time.GetTick(), s);
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{

	auto mouse_pos = TVec2(xpos, ypos);
	//cam->MouseMove(mouse_pos[0], mouse_pos[1]);
	//SetCursorPos(100, 100);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
}

int main(int argc, char** argv)
{
	int width, height;
	GLFWwindow* window;

	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	width = 640;
	height = 480;

	viewport_width = width;
	viewport_height = height;	

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

	glfwSetTime(0.0);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene(window, glfwGetTime());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete render;

	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}