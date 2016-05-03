#pragma once

#if defined(WIN32)||defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <GL/wglew.h>
#include <windows.h>
#else
#include <stdio.h>
#include <stdlib.h>
//#include <X11/X.h>
//#include <X11/Xlib.h>
//#include <GL/gl.h>
//#include <GL/glx.h>
//#include <GL/glu.h>
#include <GL/glew.h>
//#include <GL/glut.h>
#endif

namespace BaluRender
{

	const GLuint targets[] =
	{
		GL_TEXTURE_1D,
		GL_TEXTURE_2D,
		GL_TEXTURE_3D,
		GL_TEXTURE_CUBE_MAP
	};

	const GLuint formats[] =
	{
		GL_RGB,
		GL_RGBA,
		GL_LUMINANCE,
		GL_RGB16F_ARB,
		GL_DEPTH_COMPONENT24,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
	};

	inline TFormat GetFormat(GLint gl_format)
	{
		for (int i = 0; i < 8; i++)
			if (formats[i] == gl_format)
				return (TFormat)i;
	}



	class TBaluRenderInternal
	{
	public:
	  #if defined(WIN32)||defined(_WIN32)
		HDC hDC;
		HGLRC hRC;
		HWND hWnd;
		int pixel_format;
#else
		
#endif
		TVec2i screen_size;
		TMatrix4 modelview;
		TMatrix4 projection;

		//render info
		int max_aniso;
		int max_texture_units;
		int max_vertex_texture_image_units;
		int max_texture_image_units;
		int major, minor;
	};
}
