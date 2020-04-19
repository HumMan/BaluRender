
#include <baluRender.h>

using namespace BaluRender;

#define NOMINMAX

#include "baluRenderCommon.h"

#include <IL/ilut.h>

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP;

using namespace TBaluRenderEnums;

#if defined(WIN32)||defined(_WIN32)
#else
void strcpy_s(char* buf, char* value)
{
	strcpy(buf,value);
}
void strcpy_s(char* buf, int len, char* value)
{
	strcpy(buf,value);
}

#define sprintf_s sprintf
#define sscanf_s sscanf
#endif

const char* GetGLErrorString(GLenum errorCode){
	if (errorCode == 0) {
		return (const char *) "No error";
	}
	if ((errorCode >= GL_INVALID_ENUM) && (errorCode <= GL_OUT_OF_MEMORY)) {
		return (const  char *) glErrorStrings[errorCode - GL_INVALID_ENUM];
	}
	return (const char *) "Generic error";
}

void BaluRender::CheckGLError()
{
	GLenum err;
	if((err=glGetError())!=GL_NO_ERROR)
	{
		const char* err_string = GetGLErrorString(err);
		LOG(ERROR) << err_string;
	}
}

struct TStreamsDesc::TStreamsDescState
{
	struct TStreamDesc
	{
		int size;
		TBaluRenderEnums::TDataType type;
		void* data;
		TVertexBufferId vert_buf;
		TTextureId tex;
		TStreamDesc()
		{
			size = 0;
			data = 0;
		}
	};
	const static int max_tex_units = 32;
	TStreamDesc vertex, normal, color;
	TStreamDesc index;
	TStreamDesc tex[max_tex_units];
	int tex_units_usage_mask;

	void AddStream(TStream use_stream,
		int use_tex_unit, TDataType use_type,
		int use_size, void* use_data, TVertexBufferId use_buf)
	{
		TStreamDesc t;
		t.data = use_data;
		t.size = use_size;
		t.type = use_type;
		t.vert_buf = use_buf;
		switch (use_stream)
		{
		case TStream::Index:
			index = t;
			break;
		case TStream::Color:
			color = t;
			break;
		case TStream::Normal:
			normal = t;
			break;
		case TStream::Vertex:
			vertex = t;
			break;
		case TStream::TexCoord:
			tex[use_tex_unit] = t;
			break;
		default:
			assert(false);
		}
	}
};

TStreamsDesc::TStreamsDesc()
{
	state = new TStreamsDesc::TStreamsDescState();
}

TStreamsDesc::~TStreamsDesc()
{
	delete state;
}

void TStreamsDesc::AddStream(TBaluRenderEnums::TStream use_stream, int use_tex_unit, TBaluRenderEnums::TDataType use_type, int use_size, void* use_data, TVertexBufferId use_buf)
{
	state->AddStream(use_stream, use_tex_unit, use_type, use_size, use_data, use_buf);
}

void TStreamsDesc::AddStream(TStream use_stream,int use_tex_unit,TDataType use_type, int use_size, void* use_data)
{
	AddStream(use_stream,use_tex_unit,use_type,use_size,use_data,TVertexBufferId());
}

void TStreamsDesc::AddStream(TStream use_stream,int use_tex_unit,TDataType use_type, TVertexBufferId use_buf)
{
	AddStream(use_stream,use_tex_unit,use_type,0,0,use_buf);
}

void TStreamsDesc::AddStream(TStream use_stream,TDataType use_type, int use_size, void* use_data)
{
	AddStream(use_stream,0,use_type,use_size,use_data,TVertexBufferId());
}

void TStreamsDesc::AddStream(TStream use_stream,TDataType use_type, int use_size, TVertexBufferId use_buf)
{
	AddStream(use_stream,0,use_type,use_size,0,use_buf);//TODO проверять размер
}

void TStreamsDesc::AddTexture(int use_tex_unit,TTextureId use_tex_id)
{
	state->tex[use_tex_unit].tex=use_tex_id;
}

void TStreamsDesc::Clear()
{
	for(int i=0;i<state->max_tex_units;i++)
	{
		state->tex[i]= TStreamsDescState::TStreamDesc();
	}
	state->vertex= TStreamsDescState::TStreamDesc();
	state->color= TStreamsDescState::TStreamDesc();
	state->normal= TStreamsDescState::TStreamDesc();
}

bool TokenExists(char* use_string,char* use_token)
{
	/*char* curr=NULL;
	char* token=strtok_s(use_string," ",&curr);
	while(token!=NULL)
	{
		if(strstr(use_string,use_token)!=NULL)
			return false;
		token=strtok_s(NULL," ",&curr);
	}
	return false;*/
	return strstr(use_string,use_token)!=NULL;
}

void TBaluRender::LogInfo(const char* text, ...)
{
	va_list args;
	va_start(args, text);

	vsprintf_s(p->log_buff, text, args);
	LOG(INFO) << p->log_buff;

	va_end(args);
}

void TBaluRender::InitInfo()
{
	LogInfo("OpenGL INFO START:");

	int ext_len=strlen((char *)glGetString(GL_EXTENSIONS));
	char* ext=new char[ext_len+1]; 
	char* gl_ext_string = (char *)glGetString(GL_EXTENSIONS);
	strcpy_s(ext, ext_len+1,gl_ext_string);

	const char* version = (const char*)glGetString( GL_VERSION );
	sscanf_s(version, "%d.%d", &p->major, &p->minor);
	LogInfo(version);

	LogInfo((const char*)glGetString( GL_RENDERER));
	LogInfo((const char*)glGetString( GL_VENDOR));

	Support.multitexturing			=TokenExists(ext,"GL_ARB_multitexture");
	
	if(Support.multitexturing)
	{
		glGetIntegerv(GL_MAX_TEXTURE_UNITS, &p->max_texture_units);
		LogInfo("Max texture units = %i", p->max_texture_units);
		
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &p->max_vertex_texture_image_units);
		LogInfo("Max vertex texture units = %i", p->max_vertex_texture_image_units);

		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &p->max_texture_image_units);
		LogInfo("Max texture image units = %i", p->max_texture_image_units);
	}

	{
		int actualbits;
		glGetIntegerv(GL_DEPTH_BITS, &actualbits);
		LogInfo("Depth bits = %i", actualbits);
	}

	Support.vertex_array = (p->major * 10 + p->minor >= 11);
	Support.vertex_buffer			=TokenExists(ext,"GL_ARB_vertex_buffer_object");
	Support.frame_buffer			=TokenExists(ext,"GL_ARB_framebuffer_object");
	Support.vertex_program			=TokenExists(ext,"GL_ARB_vertex_program");
	Support.fragment_program		=TokenExists(ext,"GL_ARB_fragment_program");
	Support.texture_compression		=TokenExists(ext,"GL_ARB_texture_compression");
	Support.hw_generate_mipmap		=TokenExists(ext,"GL_SGIS_generate_mipmap");
	Support.anisotropic_filter		=TokenExists(ext,"GL_EXT_texture_filter_anisotropic");

	if(Support.anisotropic_filter)
	{
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &p->max_aniso);
		LogInfo("Max aniso = %i", p->max_aniso);
	}

	Support.npot_texture			=TokenExists(ext,"GL_ARB_texture_non_power_of_two");
	Support.blending_ext			=TokenExists(ext,"GL_ARB_imaging");
	Support.high_level_shading		=TokenExists(ext,"GL_ARB_shader_objects")
									&&TokenExists(ext,"GL_ARB_shading_language_100")
									&&TokenExists(ext,"GL_ARB_vertex_shader")
									&&TokenExists(ext,"GL_ARB_fragment_shader");

	char* c=ext;
	for(int i=0;i<ext_len;i++)
	{
		if(*c==' ')*c='\n';
		c++;
	}

	LogInfo(ext);

	delete[] ext;

	LogInfo("OpenGL INFO END:");
}

void TBaluRender::Initialize(TVec2i use_size)
{
	LogInfo("Initialization...");

	LogInfo("Loading image library...");
	
	CheckGLError();

	ilInit();
	iluInit();
	ilutInit();

	CheckGLError();

	ilEnable(IL_CONV_PAL);
	ilutRenderer(ILUT_OPENGL);
	ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
	ilutEnable(ILUT_GL_AUTODETECT_TEXTURE_TARGET);
	ilutEnable(ILUT_OPENGL_CONV);
	ilutEnable(ILUT_GL_USE_S3TC);

	LogInfo(" passed");

	CheckGLError();
	Set.r=this;
	Get.r = this;
	Texture.r=this;
	Shader.r=this;
	VertexBuffer.r=this;
	Support.r=this;
	ScissorRect.r = this;

	p->vertex_buffers_emul.New();//because in OpenGL indices start from 1

	p->screen_size = use_size;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		LogInfo("Error: %s", glewGetErrorString(err));
	}

	LogInfo("Status: Using GLEW %s", glewGetString(GLEW_VERSION));

	InitInfo();

	glViewport(0, 0, p->screen_size[0], p->screen_size[1]);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	p->projection.SetIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	p->modelview.SetIdentity();
	CheckGLError();
}

TBaluRender::TBaluRender(TVec2i use_size):
	Blend(this)
{
	p = new TPrivate();
	Initialize(use_size);
}

TBaluRender::~TBaluRender()
{
	ilShutDown();
	delete p;
}

TVec2i TBaluRender::ScreenSize(){
	return p->screen_size;
}

TVec2 TBaluRender::WindowToClipSpace(int x,int y){
	return TVec2(x / float(p->screen_size[0]), 1.0f - y / float(p->screen_size[1]))*2.0 - TVec2(1.0f, 1.0f);
}

void TBaluRender::Clear(bool color, bool depth)
{
	glClear(
		(color?GL_COLOR_BUFFER_BIT:0)|
		(depth?GL_DEPTH_BUFFER_BIT:0)
		);
}

unsigned int* TBaluRender::LoadImageData(const std::string& path, unsigned int& width, unsigned int& height)
{
	ILuint handle;
	ilGenImages(1, &handle);
	ilBindImage(handle);
	if (ilLoadImage(path.c_str()))
	{
		auto w = ilGetInteger(IL_IMAGE_WIDTH);
		auto h = ilGetInteger(IL_IMAGE_HEIGHT);
		size_t memory_needed = w * h * sizeof(unsigned int);
		ILuint * data = new ILuint[memory_needed];
		ilCopyPixels(0, 0, 0, w, h, 1, IL_ALPHA, IL_UNSIGNED_INT, data);


		int temp = std::numeric_limits<unsigned int>().max() / 255;
		for (int i = 0; i < w * h; i++)
			data[i] = data[i] / temp;

		width = w;
		height = h;

		ilDeleteImage(handle);
		return data;
	}
	else
	{
		auto err = ilGetError();
		auto err_string = iluErrorString(err);
		ilDeleteImage(handle);
		return nullptr;
	}
}

void TBaluRender::Draw(const TStreamsDesc& use_streams,TPrimitive use_primitive, int use_vertices_count)
{
	auto& streams = *use_streams.state;
	if(Support.vertex_buffer)
	{
		if(streams.vertex.data||streams.vertex.vert_buf.id!=0)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			if(streams.vertex.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,streams.vertex.vert_buf.id);
			glVertexPointer(streams.vertex.size, data_types[(int)streams.vertex.type], 0, streams.vertex.data);
		}
		if(streams.normal.data||streams.normal.vert_buf.id!=0)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			if(streams.normal.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,streams.normal.vert_buf.id);
			glNormalPointer(data_types[(int)streams.normal.type], 0, streams.normal.data);
		}
		if(streams.color.data||streams.color.vert_buf.id!=0)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			if(streams.color.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,streams.color.vert_buf.id);
			glColorPointer(streams.color.size, data_types[(int)streams.color.type], 0, streams.color.data);
		}
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::TStreamsDescState::max_tex_units;i++)
			{
				if(streams.tex[i].data||streams.tex[i].vert_buf.id!=0)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					if(streams.tex[i].vert_buf.id!=0)
						glBindBufferARB(GL_ARRAY_BUFFER,streams.tex[i].vert_buf.id);
					glTexCoordPointer(streams.tex[i].size, data_types[(int)streams.tex[i].type], 0, streams.tex[i].data);
				}
			}
		}else
		{
			if(streams.tex[0].data||streams.tex[0].vert_buf.id!=0)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				if(streams.tex[0].vert_buf.id!=0)
					glBindBufferARB(GL_ARRAY_BUFFER,streams.tex[0].vert_buf.id);
				glTexCoordPointer(streams.tex[0].size, data_types[(int)streams.tex[0].type], 0, streams.tex[0].data);
			}
		}
		CheckGLError();
		if(streams.index.data)
		{
			glDrawElements(primitive[(int)use_primitive], use_vertices_count, data_types[(int)streams.index.type], streams.index.data);
		}
		else if(streams.index.vert_buf.id!=0)
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER,streams.index.vert_buf.id);
			glDrawElements(primitive[(int)use_primitive], use_vertices_count, data_types[(int)streams.index.type], 0);
		}
		else
			glDrawArrays(primitive[(int)use_primitive], 0, use_vertices_count);

		CheckGLError();
		glBindBufferARB(GL_ARRAY_BUFFER,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER,0);

		if(streams.vertex.data||streams.vertex.vert_buf.id!=0)
			glDisableClientState(GL_VERTEX_ARRAY);
		if(streams.normal.data||streams.normal.vert_buf.id!=0)
			glDisableClientState(GL_NORMAL_ARRAY);
		if(streams.color.data||streams.color.vert_buf.id!=0)
			glDisableClientState(GL_COLOR_ARRAY);
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::TStreamsDescState::max_tex_units;i++)
			{
				if(streams.tex[i].data)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}
		}else
		{
			if(streams.tex[0].data)
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}else if(Support.vertex_array)
	{
		if(streams.vertex.data||streams.vertex.vert_buf.id!=0)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			if(streams.vertex.vert_buf.id!=0)
				glVertexPointer(streams.vertex.size, data_types[(int)streams.vertex.type], 0, p->vertex_buffers_emul[streams.vertex.vert_buf.id].data_pointer);
			else
				glVertexPointer(streams.vertex.size, data_types[(int)streams.vertex.type], 0, streams.vertex.data);
		}
		if(streams.normal.data||streams.normal.vert_buf.id!=0)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			if(streams.normal.vert_buf.id!=0)
				glNormalPointer(data_types[(int)streams.normal.type], 0, p->vertex_buffers_emul[streams.normal.vert_buf.id].data_pointer);
			else
				glNormalPointer(data_types[(int)streams.normal.type], 0, streams.normal.data);
		}
		if(streams.color.data||streams.color.vert_buf.id!=0)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			if(streams.color.vert_buf.id!=0)
				glColorPointer(streams.color.size, data_types[(int)streams.color.type], 0, p->vertex_buffers_emul[streams.color.vert_buf.id].data_pointer);
			else
				glColorPointer(streams.color.size, data_types[(int)streams.color.type], 0, streams.color.data);
		}
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::TStreamsDescState::max_tex_units;i++)
			{
				if(streams.tex[i].data||streams.tex[i].vert_buf.id!=0)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					if(streams.tex[i].vert_buf.id!=0)
						glTexCoordPointer(streams.tex[i].size, data_types[(int)streams.tex[i].type], 0, p->vertex_buffers_emul[streams.tex[i].vert_buf.id].data_pointer);
					else
						glTexCoordPointer(streams.tex[i].size, data_types[(int)streams.tex[i].type], 0, streams.tex[i].data);
				}
			}
		}else
		{
			if(streams.tex[0].data||streams.tex[0].vert_buf.id!=0)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				if(streams.tex[0].vert_buf.id!=0)
					glTexCoordPointer(streams.tex[0].size, data_types[(int)streams.tex[0].type], 0, p->vertex_buffers_emul[streams.tex[0].vert_buf.id].data_pointer);
				else
					glTexCoordPointer(streams.tex[0].size, data_types[(int)streams.tex[0].type], 0, streams.tex[0].data);
			}
		}
		CheckGLError();
		if(streams.index.data)
			glDrawElements(primitive[(int)use_primitive], use_vertices_count, data_types[(int)streams.index.type], streams.index.data);
		else
			glDrawArrays(primitive[(int)use_primitive], 0, use_vertices_count);
		CheckGLError();
		if(streams.vertex.data||streams.vertex.vert_buf.id!=0)
			glDisableClientState(GL_VERTEX_ARRAY);
		if(streams.normal.data||streams.normal.vert_buf.id!=0)
			glDisableClientState(GL_NORMAL_ARRAY);
		if(streams.color.data||streams.color.vert_buf.id!=0)
			glDisableClientState(GL_COLOR_ARRAY);
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::TStreamsDescState::max_tex_units;i++)
			{
				if(streams.tex[i].data)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}
		}else
		{
			if(streams.tex[0].data)
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
}

bool TBaluRender::TSet::Viewport(TVec2i use_size)
{
	auto old_viewport = r->Get.Viewport();
	if (use_size != old_viewport)
	{
		r->p->screen_size = use_size;
		glViewport(0, 0, use_size[0], use_size[1]);
		return true;
	}
	else
	{
		return false;
	}
}

void TBaluRender::TSet::Color(float r,float g,float b)
{
	auto temp = TVec3(r,g,b);
	glColor3fv((GLfloat*)&temp);
}

void TBaluRender::TSet::Color(float r,float g,float b,float a)
{
	auto temp = TVec4(r,g,b,a);
	glColor4fv((GLfloat*)&temp);
}

void TBaluRender::TSet::ClearColor(float r,float g,float b,float a)
{
	glClearColor(r,g,b,a);
}

void TBaluRender::TSet::ModelView(const TMatrix4& use_modelview)
{
	glLoadMatrixf((GLfloat*)&use_modelview);
}

void TBaluRender::TSet::ModelView(const TMatrix4d& use_modelview)
{
	glLoadMatrixd((GLdouble*)&use_modelview);
}

void TBaluRender::TSet::Projection(const TMatrix4d& use_proj)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd((GLdouble*)&use_proj);
	glMatrixMode(GL_MODELVIEW);
}

void TBaluRender::TSet::Projection(const TMatrix4& use_proj)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((GLfloat*)&use_proj);
	glMatrixMode(GL_MODELVIEW);
}

TMatrix4 TBaluRender::TGet::ModelView()
{
	TMatrix4 m;
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&m);
	return m;
}

TVec2i TBaluRender::TGet::Viewport()
{
	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	return *(TVec2i*)&m_viewport[2];
}

void TBaluRender::TDepth::Test(bool enable)
{
	if(enable)glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);
}
void TBaluRender::TDepth::Mask(bool enable)
{
	glDepthMask(enable);
}

void TBaluRender::TDepth::PolygonOffset(bool use_offset,TPolygonMode poly,float factor, float units)
{
	if(use_offset)
	{
		glEnable(internal_polygon_offset[(int)poly]);
		glPolygonOffset(factor,units);
	}
	else glDisable(internal_polygon_offset[(int)poly]);
}

void TBaluRender::TDepth::Func(char* func)
{
	assert(func!=NULL&&func[0]!='\0');
	switch(func[0])
	{
	case '1':
		assert(func[1]=='\0');
		glDepthFunc(GL_ALWAYS);break;
	case '0':
		assert(func[1]=='\0');
		glDepthFunc(GL_NEVER);break;
	case '<':
		glDepthFunc(func[1]=='='?GL_LEQUAL:GL_LESS);break;
	case '=':
		assert(func[1]=='=');
		glDepthFunc(GL_EQUAL);break;
	case '>':
		glDepthFunc(func[1]=='='?GL_GEQUAL:GL_GREATER);break;
	case '!':
		assert(func[1]=='=');
		glDepthFunc(GL_NOTEQUAL);break;
	default:assert(false);//ошибка в строке func
	}
}

void TBaluRender::TDepth::Func(TDepthFunc func)
{
	glDepthFunc(depth_funcs[func]);
}

void TBaluRender::TSet::PointSize(float size)
{
	glPointSize(size);
}

void TBaluRender::TSet::PointSmooth(bool use_smooth)
{
	if(use_smooth)
		glEnable(GL_POINT_SMOOTH);
	else
		glDisable(GL_POINT_SMOOTH);
}

void TBaluRender::TSet::ShadeModel(TShadeModel use_model)
{
	glShadeModel(internal_shade_model[(int)use_model]);
}

void TBaluRender::TSet::PolygonMode(TPolygonMode use_mode)
{
	glPolygonMode(GL_FRONT_AND_BACK, internal_polygon_mode[(int)use_mode]);
}

struct TBaluRender::TBlend::TBlendState
{
	TBaluRender* render;

	enum TTokenType
	{
		T_UNKNOWN = -1,
		T_SRCC = 0,
		T_ONE_MINUS_SRCC,
		T_SRCA,
		T_ONE_MINUS_SRCA,
		T_DSTC,
		T_ONE_MINUS_DSTC,
		T_DSTA,
		T_ONE_MINUS_DSTA,
		T_CONSTC,
		T_ONE_MINUS_CONSTC,
		T_CONSTA,
		T_ONE_MINUS_CONSTA,
		T_ONE,
		T_MINUS,
		T_PLUS,
		T_MUL,
		T_LPARENTH,
		T_RPARENTH,
		T_DONE
	};

	TTokenType tokens[15];
	int thigh;
	int c;
	
	//blend func script end
	int curr_op;
	TVec4 curr_blend_color;

	std::vector<TTokenType> buf;

	TBlendState()
	{
		buf.resize(256);
		for (size_t i = 0; i<buf.size(); i++)buf[i] = T_UNKNOWN;
		buf['s'] = T_SRCC;
		buf['d'] = T_DSTC;
		buf['c'] = T_CONSTC;
		buf['1'] = T_ONE;
		buf['-'] = T_MINUS;
		buf['+'] = T_PLUS;
		buf['*'] = T_MUL;
		buf['('] = T_LPARENTH;
		buf[')'] = T_RPARENTH;
		//
		curr_op = GL_FUNC_ADD;
		curr_blend_color = TVec4(0, 0, 0, 0);
	}

	void GetTokens(char* c, bool support_blending_ext) {
		thigh = -1;
		while (true)
		{
			tokens[++thigh] = buf[*c];
			assert((buf[*c] != T_CONSTC) || support_blending_ext);
			assert((buf[*c] != T_CONSTA) || support_blending_ext);
			assert(buf[*c] != T_UNKNOWN);
			c++;
			if (tokens[thigh] == T_SRCC || tokens[thigh] == T_DSTC || tokens[thigh] == T_CONSTC)
			{
				assert(*c == 'a' || *c == 'c');
				if (*c == 'a')
				{
					if (tokens[thigh] == T_SRCC)
						tokens[thigh] = T_SRCA;
					if (tokens[thigh] == T_DSTC)
						tokens[thigh] = T_DSTA;
					if (tokens[thigh] == T_CONSTC)
						tokens[thigh] = T_CONSTA;
				}
				c++;
			}
			if (*c == '\0') { tokens[++thigh] = T_DONE; return; }
		}
	}
	void GetToken(TTokenType token)
	{
		if (tokens[c] != token)assert(false);
		c++;
	}
	void Factor(int &factor)
	{
		switch (tokens[c])
		{
		case T_LPARENTH:
			c++;
			GetToken(T_ONE);
			GetToken(T_MINUS);
			factor = blend_equations[tokens[c] + 1];
			c++;
			GetToken(T_RPARENTH);
			break;
		default:
			factor = blend_equations[tokens[c]]; c++;
		}
	}
	bool FactorColor(int &factor)
	{
		bool result;
		switch (tokens[c])
		{
		case T_SRCC:result = true; break;
		case T_DSTC:result = false; break;
		default:assert(false);
		}
		c++;
		if (tokens[c] == T_MUL) { c++; Factor(factor); }
		else factor = GL_ONE;
		return result;
	}
};

TBaluRender::TBlend::TBlend(TBaluRender* render)
{
	blend_state = new TBlendState();
	blend_state->render = render;
}

TBaluRender::TBlend::~TBlend()
{
	delete blend_state;
}

void TBaluRender::TBlend::Enable(bool enable)
{
	if(enable)
		glEnable(GL_BLEND);
	else 
		glDisable(GL_BLEND);
}

void TBaluRender::TBlend::Func(char* func)
{
	blend_state->c=0;
	assert(func[0]!='\0');
	blend_state->GetTokens(func, blend_state->render->Support.blending_ext);
	int op,f1,f2;
	bool is_srcC= blend_state->FactorColor(f1);
	op=GL_FUNC_ADD;
	if(blend_state->tokens[blend_state->c]==TBlendState::T_DONE){
		f2=GL_ZERO;
	}else{
		switch(blend_state->tokens[blend_state->c])
		{
		case TBlendState::T_PLUS:op=GL_FUNC_ADD;break;
		case TBlendState::T_MINUS:op=GL_FUNC_SUBTRACT;break;
		default:assert(false);
		}
		blend_state->c++;
		if(is_srcC== blend_state->FactorColor(f2))assert(false);
	}
	if(is_srcC)	glBlendFunc(f1,f2);
	else		glBlendFunc(f2,f1);

	if(blend_state->curr_op==op)return;
	blend_state->curr_op=op;
	if(blend_state->render->Support.blending_ext)
		glBlendEquation(op);

}
void TBaluRender::TBlend::Func(TVec4 blend_color,char* func){
	if(!(blend_state->curr_blend_color==blend_color)&& blend_state->render->Support.blending_ext)
		glBlendColor(blend_color[0],blend_color[1],blend_color[2],blend_color[3]);
	Func(func);
}

void TBaluRender::TBlend::Func(TBlendEquation left, TBlendFunc op, TBlendEquation right)
{
	if (GLEW_VERSION_1_4)
	{
		glBlendEquation(blend_funcs[(int)op]);
		glBlendFunc(blend_equations[(int)left], blend_equations[(int)right]);
		CheckGLError();
	}
}

void TBaluRender::TAlphaTest::Enable(bool enable)
{
	if(enable)glEnable(GL_ALPHA_TEST);else glDisable(GL_ALPHA_TEST);
}

void TBaluRender::TAlphaTest::Func(char* func,float val)
{
	assert(func!=NULL&&func[0]!='\0');
	switch(func[0])
	{
	case '1':
		assert(func[1]=='\0');
		glAlphaFunc(GL_ALWAYS,val);break;
	case '0':
		assert(func[1]=='\0');
		glAlphaFunc(GL_NEVER,val);break;
	case '<':
		if(func[1]=='=')glAlphaFunc(GL_LEQUAL,val);
		else			glAlphaFunc(GL_LESS,val);break;
	case '=':
		assert(func[1]=='=');
		glAlphaFunc(GL_EQUAL,val);break;
	case '>':
		if(func[1]=='=')glAlphaFunc(GL_GEQUAL,val);
		else			glAlphaFunc(GL_GREATER,val);break;
	case '!':
		assert(func[1]=='=');
		glAlphaFunc(GL_NOTEQUAL,val);break;
	default:assert(false);//ошибка в строке func
	}
}

void TBaluRender::TAlphaTest::Func(TBaluRenderEnums::TAlphaTestFunc func, float val)
{
	glAlphaFunc(alpha_test_funcs[(int)func], val);
}

void TBaluRender::TScissorRect::Enable(bool enable)
{
	if (enable)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
}
void TBaluRender::TScissorRect::Box(TVec2i pixel_pos, TVec2i box_size)
{
	glScissor(pixel_pos[0], pixel_pos[1], box_size[0], box_size[1]);
}
