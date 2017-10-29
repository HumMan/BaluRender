#pragma once

#if defined(WIN32)||defined(_WIN32)

#include <GL/glew.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#endif

struct TGlyphPacker;

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

	struct TFrameBufferDesc
	{
		bool used;
		int width;
		int height;

		int depth_buffer_id;
		int color_buffer_id;

		bool use_color, use_depth;
		int current_render_face;
		TFrameBufferDesc() :used(false) {}
	};
	struct TVertexBufferDesc
	{
		TBaluRenderEnums::TVBType buff_type;
		void* data_pointer;
		TBaluRenderEnums::TVBUsage buff_usage;
		int size;//TODO внести на контоль в дебаге
	};
	struct TTextureDesc
	{
		bool used;
		TBaluRenderEnums::TTexType type;
		TFormat format;
		TBaluRenderEnums::TTexFilter filter;
		TBaluRenderEnums::TTexClamp clamp;
		unsigned short width, height;
		TTextureDesc() :used(false) {}
	};
	struct TShaderDesc
	{
		bool used;
		TShaderDesc() :used(false) {}
	};
	struct TBitmapFontDesc
	{
		bool used;
		unsigned int base;
		int first_char;
		int chars_count;
		TBitmapFontDesc() :used(false) {}
	};
	
}
