
#define GLEW_STATIC
#include <GL\glew.h>
#include <GL\wglew.h>

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

#include <windows.h>

class TBaluRenderInternal
{
public:
	HDC hDC;
	HGLRC hRC;
	HWND hWnd;
	int pixel_format;

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