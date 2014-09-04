#pragma once

#include <memory>

#include <baluLib.h>

#include "..\Source\images.h"

namespace TDataType
{
	enum Enum
	{
		Double,
		Float,
		Int,
		Short,
		Byte,
		UInt,
		UShort,
		UByte
	};
}

namespace TStream
{
	enum Enum
	{
		Index,
		Vertex,
		Color,
		TexCoord,
		Normal,
	};
}

namespace TVBType
{
	enum Enum
	{
		Array,
		Index,
	};
}

namespace TVBRefresh
{
	enum Enum
	{
		Stream,
		Static,
		Dynamic
	};
}

namespace TVBUsage
{
	enum Enum
	{
		Draw,
		Read,
		Copy
	};
}
namespace TVBAccess
{
	enum Enum
	{
		Read,
		Write,
		ReadWrite
	};
}

namespace TShaderType
{
	enum Enum
	{
		Vertex,
		Fragment
	};
}

namespace TTexType
{
	enum Enum
	{
		TEX_1D,
		TEX_2D,
		TEX_3D,
		CUBE
	};
}

namespace TTexFilter
{
	enum Enum
	{
		Nearest,
		Linear,
		Bilinear,
		Trilinear,
		BilinearAniso,
		TrilinearAniso
	};
}

namespace TTexClamp
{
	enum Enum
	{
		NONE	=0,
		S		=1,
		T		=2,
		R		=4,
		ST		=1|2,
		STR		=1|2|4
	};
}

namespace TPrimitive
{
	enum Enum
	{
		Points,
		Lines,
		LineLoop,
		LineStrip,
		Triangles,
		TriangleStrip,
		TriangleFan,
		Quads,
		QuadStrip
	};
}

namespace TShadeModel
{
	enum Enum
	{
		Flat,
		Smooth
	};
}

namespace TPolygonMode
{
	enum Enum
	{
		Point,
		Line,
		Fill
	};
}

struct TTextureId;
struct TVertexBufferId;
struct TFrameBufferId;
struct TShaderId;
struct TBitmapFontId;
struct TTexFontId;

class TStreamsDesc;

template <class T, int size> struct TLine;
template <class T, int size> struct TQuad;

void CheckGLError();
class TBaluRenderInternal;
class TBaluRender
{
	std::unique_ptr<TBaluRenderInternal> p;

	void InitInfo();

	struct TFrameBufferDesc
	{
		bool used;
		int width;
		int height;

		int depth_buffer_id;
		int color_buffer_id;

		bool use_color,use_depth;
		int current_render_face;
		TFrameBufferDesc():used(false){}
	};
	struct TVertexBufferDesc
	{
			TVBType::Enum buff_type;
			void* data_pointer;
			TVBUsage::Enum buff_usage;
			int size;//TODO внести на контоль в дебаге
	};
	struct TTextureDesc
	{
		bool used;
		TTexType::Enum type;
		TFormat::Enum format;
		TTexFilter::Enum filter;
		TTexClamp::Enum clamp;
		unsigned short width,height;
		TTextureDesc():used(false){}
	};
	struct TShaderDesc
	{
		bool used;
		TShaderDesc():used(false){}
	};
	struct TBitmapFontDesc
	{
		bool used;
		unsigned int base;
		int first_char;
		int chars_count;
		TBitmapFontDesc():used(false){}
	};
	struct TTexFontDesc
	{
		bool used;
		int tex;
		TTexFontDesc():used(false){}
	};
	std::vector<TFrameBufferDesc>			frame_buffers;

	std::vector<TVertexBufferDesc>			vertex_buffers;
	BaluLib::TIndexedArray<TVertexBufferDesc>vertex_buffers_emul;

	std::vector<TTextureDesc>				textures;
	std::vector<TShaderDesc>				shaders;
	std::vector<TBitmapFontDesc>			bitmap_fonts;
	std::vector<TTexFontDesc>			tex_fonts;

	void Initialize(TVec2i use_size);
public:
	TFileData log_file;
	char log_buff[102];

	TBaluRender(TVec2i use_size);
	TBaluRender(int use_window_handle,TVec2i use_size);
	~TBaluRender();

	void BeginScene();
	void EndScene();

	TVec2i ScreenSize();
	TVec2 ScreenToClipSpace(int x,int y);
	TVec2 WindowToClipSpace(int x,int y);

	void Clear(bool color=0, bool depth=1);
	void Draw(const TStreamsDesc& use_streams,TPrimitive::Enum use_primitive, int use_vertices_count);

	class TSupport
	{
		friend class TBaluRender;TBaluRender* r;
		bool multitexturing;
		bool vertex_array;
		bool vertex_buffer;
		bool frame_buffer;
		bool vertex_program;
		bool fragment_program;
		bool texture_compression;
		bool hw_generate_mipmap;
		bool anisotropic_filter;
		bool npot_texture;
		bool blending_ext;
		bool high_level_shading;

	public:
		bool VertexBuffer();
		bool FrameBuffer();
		bool VertexProgram();
		bool FragmentProgram();
		bool TextureCompression();
		bool HWGenerateMipMap();
		bool AnisotropicFilter();
		bool NPOTTexture();
		bool BlendingExt();
		bool HighLevelShading();
	}Support;

	class TCapabilities
	{
		friend class TBaluRender;TBaluRender* r;
	public:
	}Capabilities;

	class TSet
	{
		friend class TBaluRender;TBaluRender* r;
	public:
		void Viewport(TVec2i use_size);
		void VSync(bool use_vsync);
		void Color(float r, float g, float b);
		void Color(float r, float g, float b, float a);
		void ClearColor(float r,float g,float b,float a=1.0f);
		void ModelView(const TMatrix4& use_modelview);
		void Projection(const TMatrix4& use_proj);
		void ModelView(const TMatrix4d& use_modelview);
		void Projection(const TMatrix4d& use_proj);
		void PointSize(float size);
		void PointSmooth(bool use_smooth);
		void ShadeModel(TShadeModel::Enum use_model);
		void PolygonMode(TPolygonMode::Enum use_mode);
	}Set;

	class TVertexBuffer
	{
		friend class TBaluRender;TBaluRender* r;
	public:
		TVertexBufferId Create(TVBType::Enum use_type, int size, TVBRefresh::Enum use_refresh=TVBRefresh::Static, TVBUsage::Enum use_usage=TVBUsage::Draw);
		void* Map(TVertexBufferId use_id, TVBAccess::Enum use_access);
		void Unmap(TVertexBufferId use_id);
		void SubData(TVertexBufferId use_id,int use_offset,int use_size,void* use_new_data);
		void Data(TVertexBufferId use_id,int use_size,void* use_new_data);//TODO контроллировать размер и без необходимости не пересоздавать буфер
		void Delete(TVertexBufferId use_id);
	}VertexBuffer;

	class TFrameBuffer
	{
		friend class TBaluRender;TBaluRender* r;
	public:
		TFrameBufferId Create(int use_width,int use_height,bool use_color,bool use_depth);
		void CheckStatus(TFrameBufferId use_framebuf);
		void Delete(TFrameBufferId use_framebuff);
		void Resize(TFrameBufferId use_framebuff,int use_width,int use_height);
		void AttachTexture(TFrameBufferId use_framebuff,int use_draw_buff_index,TTextureId use_texture,int use_cube_side=-1);
		void AttachDepthTexture(TFrameBufferId use_framebuff,TTextureId use_texture,int use_cube_side=-1);
		//TODO надо еще отсоединять текстуры
		void Bind(TFrameBufferId use_framebuff);
		void BindMain();
		void SetDrawBuffersCount(int use_count);//TODO должно задаваться автоматически в зависимости от подсоединенных буферов цвета
	}FrameBuffer;

	class TTexture
	{
		friend class TBaluRender;TBaluRender* r;
	public:
		TTextureId Create(const char* fname);
		TTextureId Create(TTexType::Enum use_type, TFormat::Enum use_format,
			int use_width, int use_height, TTexFilter::Enum use_filter=TTexFilter::Bilinear);
		void Delete(TTextureId use_tex);
		TTextureId CreateTarget();
		void SetFilter(TTextureId use_tex, TTexFilter::Enum use_filter, 
			TTexClamp::Enum use_clamp=TTexClamp::NONE, int use_aniso=0);
		void Enable(bool enable);
		void Bind(TTextureId use_tex);
		void CopyFramebufferTo(TTextureId use_tex);
		TVec2 GetRTCoords();
	}Texture;

	class TShader
	{
		friend class TBaluRender;TBaluRender* r;
	private:
		void LoadShaderGLSL(const int use_shader,const char* use_source,const char* use_defines);
		void LoadShaderASM(const int& use_shader, const TShaderType::Enum use_type,const char* use_source,const char* use_defines);
	public:
		TShaderId Create(const char* use_file_name,const char* use_defines);
		TShaderId LoadGLSL(const char* use_vsource,const char* use_fsource,const char* use_defines);
		TShaderId LoadASM(const char* use_vsource,const char* use_fsource,const char* use_defines);
		void Delete(const TShaderId use_shader);
		void Bind(const TShaderId use_shader);
		void Unbind(const TShaderId use_shader);
		int GetUniformLocation(const TShaderId use_shader, const char* use_uniform_name);
		void SetUniform(const TShaderId use_shader, int location,const TVec3& use_vec);
		void SetUniform(const TShaderId use_shader, int location,const TVec4& use_vec);
		void SetUniform(const TShaderId use_shader, int location,const TMatrix3& use_mat);
		void SetUniform(const TShaderId use_shader, int location,const TMatrix4& use_mat);
		void SetUniform(const TShaderId use_shader, int location,int count,const TVec3& use_vec);
		void SetUniform(const TShaderId use_shader, int location,int count,const TVec4& use_vec);
		void SetUniform(const TShaderId use_shader, int location,int count,const TMatrix3& use_mat);
		void SetUniform(const TShaderId use_shader, int location,int count,const TMatrix4& use_mat);
	}Shader;

	class TDepth
	{
		friend class TBaluRender;
		TBaluRender* r;
	public:
		enum TDepthFunc
		{
			DF_ALWAYS,
			DF_NEVER,
			DF_LEQUAL,
			DF_LESS,
			DF_EQUAL,
			DF_NOTEQUAL,
			DF_GEQUAL,
			DF_GREATER
		};
		void Test(bool enable);
		void Func(char* func);//funcs: "1","0","<","<=","==",">=",">","!="
		void Func(TDepthFunc func);
		void Mask(bool enable);
		void PolygonOffset(bool use_offset,TPolygonMode::Enum poly,float factor=0, float units=0);
	}Depth;

	class TBitmapFont
	{
		friend class TBaluRender;
		TBaluRender* r;
	public:
		TBitmapFontId Create();
		void Delete(TBitmapFontId use_font);
		void Print(TBitmapFontId use_font,TVec2 pos,char* text,...);
		void Print(TBitmapFontId use_font,TVec3 pos,char* text,...);
	}BitmapFont;

	class TTexFont
	{
		friend class TBaluRender;
		TBaluRender* r;
	public:
		TTexFontId Create();
		void Delete(TTexFontId use_font);
		void Print(TTexFontId use_font,char* text,...);
	}TexFont;

	class TBlend
	{
		friend class TBaluRender;TBaluRender* r;
		//blend func script
	public:
		enum TTokenType
		{
			T_UNKNOWN=-1,
			T_SRCC=0,
			T_SRCA,
			T_DSTC,
			T_DSTA,
			T_CONSTC,
			T_CONSTA,
			T_ONE,
			T_MINUS,
			T_PLUS,
			T_MUL,
			T_LPARENTH,
			T_RPARENTH,
			T_DONE
		};
	private:
		TTokenType tokens[15];
		int thigh;
		int c;
		TBlend();		
		void GetTokens(char* c);
		void GetToken(TTokenType token);
		void Factor(int &factor);
		bool FactorColor(int &factor);
		//blend func script end
		int curr_op;
		TVec4 curr_blend_color;
	public:
		enum TBlendFunc
		{
			BF_ADD,
			BF_SUBTRACT
		};
		enum TBlendEquation
		{
			BE_SRC_COLOR,
			BE_SRC_ALPHA,
			BE_DST_COLOR,
			BE_DST_ALPHA,
			BE_CONSTANT_COLOR,
			BE_CONSTANT_ALPHA
		};
		void Enable(bool enable);
		void Func(char* func);
		//SetFunc: задает функцию смешивания
		//sc,sa - source color/alpha; dc,da - destination color/alpha; cc,ca - constant color/alpha;
		//Term : ( 'sc'
		//       | 'sa'
		//       | 'dc'
		//       | 'da'
		//       | 'cc'
		//       | 'ca'
		//       ) ;
		//ColFactor : ( 'sc'
		//            | 'dc'
		//            ) ( '*' ( Term
		//                    | '(1-' Term ')'
		//                    ))? ;
		//Func :  ColFactor ( ('+'|'-') ColFactor)? ;
		//Например:
		//"dc+sc"
		//"sc*sa+dc*(1-sa)"
		//"sc*sa-dc*(1-sc)"
		//"sc*cc"
		//"dc*sa"
		void Func(TVec4 blend_color,char* func);
		void Func(TBlendEquation left, TBlendFunc op, TBlendEquation right);
	}Blend;

	class TAlphaTest
	{
		friend class TBaluRender;TBaluRender* r;
	public:
		enum TAlphaTestFunc
		{
			AT_ALWAYS,
			AT_NEVER,
			AT_LEQUAL,
			AT_LESS,
			AT_EQUAL,
			AT_NOTEQUAL,
			AT_GEQUAL,
			AT_GREATER
		};
		void Enable(bool enable);
		void Func(char* func,float val);//funcs: "1","0","<","<=","==",">=",">","!="
		void Func(TAlphaTestFunc func, float val);
	}AlphaTest;

	//TODO ввести примитив pushAttrib чтобы не делать после каждого блока enable disable чтобы движок сам определял что изменилось и восстанавливал
	//если сделать такую вещь в текстовом виде объеденив все параметры в один эффект то получится что-то наподобии directx effect
};


struct TTextureId
{
	friend class TBaluRender::TTexture;
	friend class TBaluRender::TFrameBuffer;
private:
	int id;
public:
	TTextureId():id(0){}
	bool IsNull(){ return id != 0; }
};

struct TVertexBufferId//TODO для буферов нужно иметь полную эмуляцию при их отсутствии и т.п. для отрисовки массивов
{
	friend class TBaluRender::TVertexBuffer;
	friend class TBaluRender;
private:
	int id;//TODO должно быть приватное
public:
	TVertexBufferId():id(0){}
};

struct TShaderId
{
	friend class TBaluRender::TShader;
private:
	int id;
public:
	TShaderId():id(0){}
};

struct TFrameBufferId
{
	friend class TBaluRender::TFrameBuffer;
private:
	int id;
public:
	TFrameBufferId():id(0){}
};

struct TBitmapFontId
{
	friend class TBaluRender::TBitmapFont;
private:
	int id;
public:
	TBitmapFontId():id(0){}//TODO
};

struct TTexFontId
{
	friend class TBaluRender::TTexFont;
private:
	int id;
public:
	TTexFontId():id(0){}//TODO
};

class TStreamsDesc
{
	friend class TBaluRender;
public:
	struct TStreamDesc
	{
		int size;
		TDataType::Enum type;
		void* data;
		TVertexBufferId vert_buf;
		TTextureId tex;
		TStreamDesc()
		{
			size=0;
			data=0;
		}
	};
private:
	const static int max_tex_units=32;
	TStreamDesc vertex,normal,color;
	TStreamDesc index;
	TStreamDesc tex[max_tex_units];
	int tex_units_usage_mask;
	void AddStream(TStream::Enum use_stream,int use_tex_unit,TDataType::Enum use_type, int use_size, void* use_data, TVertexBufferId use_buf);
public:
	void AddStream(TStream::Enum use_stream,int use_tex_unit,TDataType::Enum use_type, int use_size, void* use_data);
	void AddStream(TStream::Enum use_stream,int use_tex_unit,TDataType::Enum use_type, TVertexBufferId use_buf);
	void AddStream(TStream::Enum use_stream,TDataType::Enum use_type, int use_size, void* use_data);
	void AddStream(TStream::Enum use_stream,TDataType::Enum use_type, int use_size, TVertexBufferId use_buf);
	void AddTexture(int use_tex_unit,TTextureId use_tex_id);
	void Clear();
};

template <class T,int size>
struct TLine
{
	TVec<T,size> v[2];
	void Set(const TVec<T,size> &p0)
	{
		v[0]=p0;
		v[1]=p0;
	}
	void Set(const TVec<T,size> &p0,const TVec<T,size> &p1)
	{
		v[0]=p0;
		v[1]=p1;
	}
	TVec<T,size>& operator[](int id)
	{
		assert(id>=0&&id<2);
		return v[id];
	}
};

template <class T, int size>
struct TQuad
{
	TVec<T,size> v[4];
	TVec<T,size>& operator[](int id)
	{
		assert(id>=0&&id<4);
		return v[id];
	}
	void Set(const TVec2 &pos,TVec2 half_size, float z)
	{
		v[0]=TVec3(TVec2(-half_size[0],-half_size[1])+pos,z);
		v[1]=TVec3(TVec2(-half_size[0], half_size[1])+pos,z);
		v[2]=TVec3(TVec2( half_size[0], half_size[1])+pos,z);
		v[3]=TVec3(TVec2( half_size[0],-half_size[1])+pos,z);
	}
	void Set(const TVec2 &pos,TVec2 size,float angle,float z)
	{
		TVec2 s=size/2.0f;
		TMatrix2 rot(
			cosf(angle),	-sinf(angle),
			sinf(angle),	cosf(angle));

		v[0]=TVec3(rot*TVec2(-s[0],-s[1])+pos,z);
		v[1]=TVec3(rot*TVec2(-s[0], s[1])+pos,z);
		v[2]=TVec3(rot*TVec2( s[0], s[1])+pos,z);
		v[3]=TVec3(rot*TVec2( s[0],-s[1])+pos,z);
	}
	void Set(const TVec2 &pos,float size)
	{
		float s=size*0.5f;
		v[0]=TVec2(-s,-s)+pos;
		v[1]=TVec2(-s, s)+pos;
		v[2]=TVec2( s, s)+pos;
		v[3]=TVec2( s,-s)+pos;
	}
	void Set(float left, float right, float bottom, float top)
	{
		v[0]=TVec2(left,bottom);
		v[1]=TVec2(left,top);
		v[2]=TVec2( right,top);
		v[3]=TVec2( right,bottom);
	}
	void Set(const TVec2 &pos,float size,float angle)
	{
		float s=size/2.0f;
		TMatrix2 rot(
			cosf(angle),	-sinf(angle),
			sinf(angle),	cosf(angle));

		v[0]=rot*TVec2(-s,-s)+pos;
		v[1]=rot*TVec2(-s, s)+pos;
		v[2]=rot*TVec2( s, s)+pos;
		v[3]=rot*TVec2( s,-s)+pos;
	}
};