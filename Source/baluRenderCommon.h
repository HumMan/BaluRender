#pragma once

#if defined(WIN32)||defined(_WIN32)

#include <GL/glew.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#endif

class TGlyphPacker;

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


	static const char *glErrorStrings[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1] = {
		"Invalid enumerant",
		"Invalid value",
		"Invalid operation",
		"Stack overflow",
		"Stack underflow",
		"Out of memory",
	};

	const GLuint data_types[] =
	{
		GL_DOUBLE,
		GL_FLOAT,
		GL_INT,
		GL_SHORT,
		GL_BYTE,
		GL_UNSIGNED_INT,
		GL_UNSIGNED_SHORT,
		GL_UNSIGNED_BYTE,
	};


	const GLenum primitive[] =
	{
		GL_POINTS,
		GL_LINES,
		GL_LINE_LOOP,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
		GL_QUADS,
		GL_QUAD_STRIP
	};

	const GLenum internal_shade_model[] =
	{
		GL_FLAT,
		GL_SMOOTH
	};

	const GLenum internal_polygon_mode[] =
	{
		GL_POINT,
		GL_LINE,
		GL_FILL
	};

	const GLenum internal_polygon_offset[] =
	{
		GL_POLYGON_OFFSET_POINT,
		GL_POLYGON_OFFSET_LINE,
		GL_POLYGON_OFFSET_FILL
	};

	const GLenum depth_funcs[] =
	{
		GL_ALWAYS,
		GL_NEVER,
		GL_LEQUAL,
		GL_LESS,
		GL_EQUAL,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_GREATER
	};

	const GLenum alpha_test_funcs[] =
	{
		GL_ALWAYS,
		GL_NEVER,
		GL_LEQUAL,
		GL_LESS,
		GL_EQUAL,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_GREATER
	};

	static const GLenum blend_equations[6 * 2] =
	{
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_CONSTANT_COLOR,
		GL_ONE_MINUS_CONSTANT_COLOR,
		GL_CONSTANT_ALPHA,
		GL_ONE_MINUS_CONSTANT_ALPHA
	};

	//TODO добавить другие функции https://www.khronos.org/opengles/sdk/docs/man/xhtml/glBlendFunc.xml
	static const GLenum blend_funcs[6] =
	{
		GL_FUNC_ADD,
		GL_FUNC_SUBTRACT
	};

	inline TBaluRenderEnums::TFormat GetFormat(GLint gl_format)
	{
		for (int i = 0; i < 8; i++)
			if (formats[i] == gl_format)
				return (TBaluRenderEnums::TFormat)i;
	}


		class TBaluRender::TPrivate
		{
		public:
			char log_buff[10240];

			TVec2i screen_size;
			TMatrix4 modelview;
			TMatrix4 projection;

			//render info
			int max_aniso;
			int max_texture_units;
			int max_vertex_texture_image_units;
			int max_texture_image_units;
			int major, minor;

			std::vector<TFrameBufferDesc>			frame_buffers;

			std::vector<TVertexBufferDesc>			vertex_buffers;
			BaluLib::TIndexedArray<TVertexBufferDesc>vertex_buffers_emul;

			std::vector<TTextureDesc>				textures;
			std::vector<TShaderDesc>				shaders;
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
		TBaluRenderEnums::TFormat format;
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
}
