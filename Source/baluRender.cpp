#include <baluRender.h>


extern TFileData log_file("log.txt","w+");

static const char *glErrorStrings[GL_OUT_OF_MEMORY - GL_INVALID_ENUM + 1] = {
	"Invalid enumerant",
	"Invalid value",
	"Invalid operation",
	"Stack overflow",
	"Stack underflow",
	"Out of memory",
};

#ifndef _DEBUG
#define _DEBUG 0
#endif

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


const GLenum primitive[]=
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

const GLenum internal_shade_model[]=
{
	GL_FLAT,
	GL_SMOOTH
};

const GLenum internal_polygon_mode[]=
{
	GL_POINT,
	GL_LINE,
	GL_FILL
};

const GLenum internal_polygon_offset[]=
{
	GL_POLYGON_OFFSET_POINT,
	GL_POLYGON_OFFSET_LINE,
	GL_POLYGON_OFFSET_FILL
};

const GLenum depth_funcs[]=
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

static const GLenum blend_equations[6] =
{
	GL_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_DST_COLOR,
	GL_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA
};

//TODO добавить другие функции https://www.khronos.org/opengles/sdk/docs/man/xhtml/glBlendFunc.xml
static const GLenum blend_funcs[6] =
{
	GL_ADD,
	GL_SUBTRACT
};

const char* GetGLErrorString(GLenum errorCode){
	if (errorCode == 0) {
		return (const char *) "No error";
	}
	if ((errorCode >= GL_INVALID_ENUM) && (errorCode <= GL_OUT_OF_MEMORY)) {
		return (const  char *) glErrorStrings[errorCode - GL_INVALID_ENUM];
	}
	return (const char *) "Generic error";
}

void CheckGLError()
{
	GLenum err;
	if((err=glGetError())!=GL_NO_ERROR){
		MessageBoxA(0,GetGLErrorString(err),"OpenGL error!",MB_OK|MB_ICONERROR);
		assert(0);
	}
}

void TStreamsDesc::AddStream(TStream::Enum use_stream,
							 int use_tex_unit,TDataType::Enum use_type,
							 int use_size, void* use_data, TVertexBufferId use_buf)
{
	TStreamDesc t;
	t.data = use_data;
	t.size=use_size;
	t.type = use_type;
	t.vert_buf =use_buf;
	switch(use_stream)
	{
	case TStream::Index:
		index=t;
		break;
	case TStream::Color:
		color=t;
		break;
	case TStream::Normal:
		normal=t;
		break;
	case TStream::Vertex:
		vertex=t;
		break;
	case TStream::TexCoord:
		tex[use_tex_unit]=t;
		break;
	default:
		assert(false);
	}
}

void TStreamsDesc::AddStream(TStream::Enum use_stream,int use_tex_unit,TDataType::Enum use_type, int use_size, void* use_data)
{
	AddStream(use_stream,use_tex_unit,use_type,use_size,use_data,TVertexBufferId());
}

void TStreamsDesc::AddStream(TStream::Enum use_stream,int use_tex_unit,TDataType::Enum use_type, TVertexBufferId use_buf)
{
	AddStream(use_stream,use_tex_unit,use_type,0,0,use_buf);
}

void TStreamsDesc::AddStream(TStream::Enum use_stream,TDataType::Enum use_type, int use_size, void* use_data)
{
	AddStream(use_stream,0,use_type,use_size,use_data,TVertexBufferId());
}

void TStreamsDesc::AddStream(TStream::Enum use_stream,TDataType::Enum use_type, int use_size, TVertexBufferId use_buf)
{
	AddStream(use_stream,0,use_type,use_size,0,use_buf);//TODO проверять размер
}

void TStreamsDesc::AddTexture(int use_tex_unit,TTextureId use_tex_id)
{
	tex[use_tex_unit].tex=use_tex_id;
}

void TStreamsDesc::Clear()
{
	for(int i=0;i<max_tex_units;i++)
	{
		tex[i]=TStreamDesc();
	}
	vertex=TStreamDesc();
	color=TStreamDesc();
	normal=TStreamDesc();
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

void TBaluRender::InitInfo()
{
	sprintf_s(log_buff,"OpenGL INFO START:\n");log_file.Write(log_buff);

	int ext_len=strlen((char *)glGetString(GL_EXTENSIONS));
	char* ext=new char[ext_len+1]; 
	char* gl_ext_string = (char *)glGetString(GL_EXTENSIONS);
	strcpy_s(ext, ext_len+1,gl_ext_string);

	const char* version = (const char*)glGetString( GL_VERSION );
    sscanf_s( version, "%d.%d", &major, &minor );
	log_file.Write(version);

	log_file.Write((const char*)glGetString( GL_RENDERER));
	log_file.Write((const char*)glGetString( GL_VENDOR));

	Support.multitexturing			=TokenExists(ext,"GL_ARB_multitexture");
	
	if(Support.multitexturing)
	{
		//TODO убрать комментарии!!!!!
		//glGetIntegerv(GL_MAX_TEXTURE_UNITS,&max_texture_units);
		sprintf_s(log_buff,"Max texture units = %i\n",max_texture_units);log_file.Write(log_buff);
	}

	Support.vertex_array			=(major*10+minor>=11);
	Support.vertex_buffer			=TokenExists(ext,"GL_ARB_vertex_buffer_object");
	Support.frame_buffer			=TokenExists(ext,"GL_ARB_framebuffer_object");
	Support.vertex_program			=TokenExists(ext,"GL_ARB_vertex_program");
	Support.fragment_program		=TokenExists(ext,"GL_ARB_fragment_program");
	Support.texture_compression		=TokenExists(ext,"GL_ARB_texture_compression");
	Support.hw_generate_mipmap		=TokenExists(ext,"GL_SGIS_generate_mipmap");
	Support.anisotropic_filter		=TokenExists(ext,"GL_EXT_texture_filter_anisotropic");

	if(Support.anisotropic_filter)
	{
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&max_aniso);
		sprintf_s(log_buff,"Max aniso = %i\n",max_aniso);log_file.Write(log_buff);
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
	log_file.Write(ext);

	delete[] ext;

	sprintf_s(log_buff,"\nOpenGL INFO END:\n");log_file.Write(log_buff);
}

void TBaluRender::Initialize(TVec2i use_size)
{
	sprintf_s(log_buff,"Initialization...");log_file.Write(log_buff);
	CheckGLError();
	Set.r=this;
	Texture.r=this;
	Shader.r=this;
	VertexBuffer.r=this;
	Support.r=this;
	Blend.r=this;
	BitmapFont.r=this;
	TexFont.r=this;

	vertex_buffers_emul.New();//because in OpenGL indices start from 1

	screen_size=use_size;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	sprintf_s(log_buff," passed\n");log_file.Write(log_buff);

	InitInfo();

	glViewport(0,0,screen_size[0],screen_size[1]);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	projection.SetIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	modelview.SetIdentity();
	CheckGLError();
}

TBaluRender::TBaluRender(TVec2i use_size)
{
	Initialize(use_size);
}

TBaluRender::TBaluRender(HWND use_window_handle,TVec2i use_size)
{
	sprintf_s(log_buff,"Context creation...");log_file.Write(log_buff);

	hWnd=use_window_handle;
	hDC = GetDC (hWnd);
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory (&pfd, sizeof (pfd));
	pfd.nSize = sizeof (pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pixel_format = ChoosePixelFormat (hDC, &pfd);
	SetPixelFormat (hDC, pixel_format, &pfd);
	hRC = wglCreateContext((HDC)hDC);
	if(!wglMakeCurrent(hDC,hRC))assert(false);
	CheckGLError();
	if(!wglMakeCurrent(hDC,hRC))assert(false);
	CheckGLError();
	sprintf_s(log_buff," passed\n");log_file.Write(log_buff);

	Initialize(use_size);
}

TBaluRender::~TBaluRender()
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext((HGLRC)hRC);
	ReleaseDC((HWND)hWnd, (HDC)hDC);
}

void TBaluRender::BeginScene()
{
	if(!wglMakeCurrent((HDC)hDC,(HGLRC)hRC))assert(false);
	CheckGLError();
}
void TBaluRender::EndScene()
{
	SwapBuffers((HDC)hDC);
}

TVec2i TBaluRender::ScreenSize(){
	return screen_size;
}

TVec2 TBaluRender::ScreenToClipSpace(int x,int y){
	tagPOINT p;
	p.x=x;
	p.y=y;
	ScreenToClient((HWND)hWnd,&p);
	return TVec2(p.x/float(screen_size[0]),1.0-p.y/float(screen_size[1]))*2.0-TVec2(1.0,1.0);
}

TVec2 TBaluRender::WindowToClipSpace(int x,int y){
	return TVec2(x/float(screen_size[0]),1.0-y/float(screen_size[1]))*2.0-TVec2(1.0,1.0);
}

void TBaluRender::Clear(bool color, bool depth)
{
	glClear(
		(color?GL_COLOR_BUFFER_BIT:0)|
		(depth?GL_DEPTH_BUFFER_BIT:0)
		);
}

void TBaluRender::Draw(const TStreamsDesc& use_streams,TPrimitive::Enum use_primitive, int use_vertices_count)
{
	if(Support.vertex_buffer)
	{
		if(use_streams.vertex.data||use_streams.vertex.vert_buf.id!=0)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			if(use_streams.vertex.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,use_streams.vertex.vert_buf.id);
			glVertexPointer(use_streams.vertex.size,data_types[use_streams.vertex.type],0,use_streams.vertex.data);
		}
		if(use_streams.normal.data||use_streams.normal.vert_buf.id!=0)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			if(use_streams.normal.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,use_streams.normal.vert_buf.id);
			glNormalPointer(data_types[use_streams.normal.type],0,use_streams.normal.data);
		}
		if(use_streams.color.data||use_streams.color.vert_buf.id!=0)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			if(use_streams.color.vert_buf.id!=0)
				glBindBufferARB(GL_ARRAY_BUFFER,use_streams.color.vert_buf.id);
			glColorPointer(use_streams.color.size,data_types[use_streams.color.type],0,use_streams.color.data);
		}
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::max_tex_units;i++)
			{
				if(use_streams.tex[i].data||use_streams.tex[i].vert_buf.id!=0)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					if(use_streams.tex[i].vert_buf.id!=0)
						glBindBufferARB(GL_ARRAY_BUFFER,use_streams.tex[i].vert_buf.id);
					glTexCoordPointer(use_streams.tex[i].size,data_types[use_streams.tex[i].type],0,use_streams.tex[i].data);
				}
			}
		}else
		{
			if(use_streams.tex[0].data||use_streams.tex[0].vert_buf.id!=0)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				if(use_streams.tex[0].vert_buf.id!=0)
					glBindBufferARB(GL_ARRAY_BUFFER,use_streams.tex[0].vert_buf.id);
				glTexCoordPointer(use_streams.tex[0].size,data_types[use_streams.tex[0].type],0,use_streams.tex[0].data);
			}
		}
		CheckGLError();
		if(use_streams.index.data)
		{
			glDrawElements(primitive[use_primitive],use_vertices_count,data_types[use_streams.index.type],use_streams.index.data);
		}
		else if(use_streams.index.vert_buf.id!=0)
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER,use_streams.index.vert_buf.id);
			glDrawElements(primitive[use_primitive],use_vertices_count,data_types[use_streams.index.type],0);
		}
		else
			glDrawArrays(primitive[use_primitive],0,use_vertices_count);

		CheckGLError();
		glBindBufferARB(GL_ARRAY_BUFFER,0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER,0);

		if(use_streams.vertex.data||use_streams.vertex.vert_buf.id!=0)
			glDisableClientState(GL_VERTEX_ARRAY);
		if(use_streams.normal.data||use_streams.normal.vert_buf.id!=0)
			glDisableClientState(GL_NORMAL_ARRAY);
		if(use_streams.color.data||use_streams.color.vert_buf.id!=0)
			glDisableClientState(GL_COLOR_ARRAY);
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::max_tex_units;i++)
			{
				if(use_streams.tex[i].data)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}
		}else
		{
			if(use_streams.tex[0].data)
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}else if(Support.vertex_array)
	{
		if(use_streams.vertex.data||use_streams.vertex.vert_buf.id!=0)
		{
			glEnableClientState(GL_VERTEX_ARRAY);
			if(use_streams.vertex.vert_buf.id!=0)
				glVertexPointer(use_streams.vertex.size,data_types[use_streams.vertex.type],0,vertex_buffers_emul[use_streams.vertex.vert_buf.id].data_pointer);
			else
				glVertexPointer(use_streams.vertex.size,data_types[use_streams.vertex.type],0,use_streams.vertex.data);
		}
		if(use_streams.normal.data||use_streams.normal.vert_buf.id!=0)
		{
			glEnableClientState(GL_NORMAL_ARRAY);
			if(use_streams.normal.vert_buf.id!=0)
				glNormalPointer(data_types[use_streams.normal.type],0,vertex_buffers_emul[use_streams.normal.vert_buf.id].data_pointer);
			else
				glNormalPointer(data_types[use_streams.normal.type],0,use_streams.normal.data);
		}
		if(use_streams.color.data||use_streams.color.vert_buf.id!=0)
		{
			glEnableClientState(GL_COLOR_ARRAY);
			if(use_streams.color.vert_buf.id!=0)
				glColorPointer(use_streams.color.size,data_types[use_streams.color.type],0,vertex_buffers_emul[use_streams.color.vert_buf.id].data_pointer);
			else
				glColorPointer(use_streams.color.size,data_types[use_streams.color.type],0,use_streams.color.data);
		}
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::max_tex_units;i++)
			{
				if(use_streams.tex[i].data||use_streams.tex[i].vert_buf.id!=0)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					if(use_streams.tex[i].vert_buf.id!=0)
						glTexCoordPointer(use_streams.tex[i].size,data_types[use_streams.tex[i].type],0,vertex_buffers_emul[use_streams.tex[i].vert_buf.id].data_pointer);
					else
						glTexCoordPointer(use_streams.tex[i].size,data_types[use_streams.tex[i].type],0,use_streams.tex[i].data);
				}
			}
		}else
		{
			if(use_streams.tex[0].data||use_streams.tex[0].vert_buf.id!=0)
			{
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				if(use_streams.tex[0].vert_buf.id!=0)
					glTexCoordPointer(use_streams.tex[0].size,data_types[use_streams.tex[0].type],0,vertex_buffers_emul[use_streams.tex[0].vert_buf.id].data_pointer);
				else
					glTexCoordPointer(use_streams.tex[0].size,data_types[use_streams.tex[0].type],0,use_streams.tex[0].data);
			}
		}
		CheckGLError();
		if(use_streams.index.data)
			glDrawElements(primitive[use_primitive],use_vertices_count,data_types[use_streams.index.type],use_streams.index.data);
		else
			glDrawArrays(primitive[use_primitive],0,use_vertices_count);
		CheckGLError();
		if(use_streams.vertex.data||use_streams.vertex.vert_buf.id!=0)
			glDisableClientState(GL_VERTEX_ARRAY);
		if(use_streams.normal.data||use_streams.normal.vert_buf.id!=0)
			glDisableClientState(GL_NORMAL_ARRAY);
		if(use_streams.color.data||use_streams.color.vert_buf.id!=0)
			glDisableClientState(GL_COLOR_ARRAY);
		if(Support.multitexturing)
		{
			for(int i=0;i<TStreamsDesc::max_tex_units;i++)
			{
				if(use_streams.tex[i].data)
				{
					glClientActiveTextureARB(GL_TEXTURE0+i);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
			}
		}else
		{
			if(use_streams.tex[0].data)
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}
}

void TBaluRender::TSet::Viewport(TVec2i use_size)
{
	r->screen_size=use_size;
	glViewport(0,0,use_size[0],use_size[1]);
}

void TBaluRender::TSet::VSync(bool use_vsync) 
{
	PFNWGLSWAPINTERVALEXTPROC wglSwapInterval = NULL;
	wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	if ( wglSwapInterval ) wglSwapInterval(use_vsync);
}

void TBaluRender::TSet::Color(float r,float g,float b)
{
	glColor3fv((GLfloat*)&TVec3(r,g,b));
}

void TBaluRender::TSet::Color(float r,float g,float b,float a)
{
	glColor4fv((GLfloat*)&TVec4(r,g,b,a));
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

void TBaluRender::TDepth::Test(bool enable)
{
	if(enable)glEnable(GL_DEPTH_TEST);
	else glDisable(GL_DEPTH_TEST);
}
void TBaluRender::TDepth::Mask(bool enable)
{
	glDepthMask(enable);
}

void TBaluRender::TDepth::PolygonOffset(bool use_offset,TPolygonMode::Enum poly,float factor, float units)
{
	if(use_offset)
	{
		glEnable ( internal_polygon_offset[poly] );
		glPolygonOffset(factor,units);
	}
	else glDisable (  internal_polygon_offset[poly] );
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

void TBaluRender::TDepth::Func(TDepth::TDepthFunc func)
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

void TBaluRender::TSet::ShadeModel(TShadeModel::Enum use_model)
{
	glShadeModel(internal_shade_model[use_model]);
}

void TBaluRender::TSet::PolygonMode(TPolygonMode::Enum use_mode)
{
	glPolygonMode(GL_FRONT_AND_BACK,internal_polygon_mode[use_mode]);
}

void TBaluRender::TBlend::GetToken(TTokenType token)
{
	if(tokens[c]!=token)assert(false);
	c++;
}
void TBaluRender::TBlend::Factor(int &factor)
{
	
	switch(tokens[c])
	{
	case T_LPARENTH:
		c++;GetToken(T_ONE);GetToken(T_MINUS);
		factor=blend_equations[tokens[c]]+1;
		c++;
		GetToken(T_RPARENTH);
		break;
	default: factor = blend_equations[tokens[c]]; c++;
	}
}

void TBaluRender::TBlend::Enable(bool enable)
{
	if(enable)glEnable(GL_BLEND);else glDisable(GL_BLEND);
}

static TBaluRender::TBlend::TTokenType buf[256];

TBaluRender::TBlend::TBlend()
{
	for(int i=0;i<256;i++)buf[i]=T_UNKNOWN;
	buf['s']=T_SRCC;
	buf['d']=T_DSTC;
	buf['c']=T_CONSTC;
	buf['1']=T_ONE;
	buf['-']=T_MINUS;
	buf['+']=T_PLUS;
	buf['*']=T_MUL;
	buf['(']=T_LPARENTH;
	buf[')']=T_RPARENTH;
	//
	curr_op=GL_FUNC_ADD;
	curr_blend_color=TVec4(0,0,0,0);
}

void TBaluRender::TBlend::GetTokens(char* c){
	thigh=-1;
	while(true)
	{
		tokens[++thigh]=buf[*c];
		assert((buf[*c]!=T_CONSTC)||r->Support.blending_ext);
		assert((buf[*c]!=T_CONSTA)||r->Support.blending_ext);
		assert(buf[*c]!=T_UNKNOWN);
		c++;
		if(tokens[thigh]==T_SRCC||tokens[thigh]==T_DSTC||tokens[thigh]==T_CONSTC)
		{
			assert(*c=='a'||*c=='c');
			*(char*)(&tokens[thigh])+=(*c=='a');
			c++;
		}
		if(*c=='\0'){tokens[++thigh]=T_DONE;return;}
	}
}

bool TBaluRender::TBlend::FactorColor(int &factor)
{
	bool result;
	switch(tokens[c])
	{
	case T_SRCC:result=true;break;
	case T_DSTC:result=false;break;	
	default:assert(false);
	}
	c++;
	if(tokens[c]==T_MUL){c++;Factor(factor);}
	else factor=GL_ONE;
	return result;
}

void TBaluRender::TBlend::Func(char* func)
{
	c=0;
	assert(func[0]!='\0');
	GetTokens(func);
	int op,f1,f2;
	bool is_srcC=FactorColor(f1);
	op=GL_FUNC_ADD;
	if(tokens[c]==T_DONE){
		f2=GL_ZERO;
	}else{
		switch(tokens[c])
		{
		case T_PLUS:op=GL_FUNC_ADD;break;
		case T_MINUS:op=GL_FUNC_SUBTRACT;break;
		default:assert(false);
		}
		c++;
		if(is_srcC==FactorColor(f2))assert(false);
	}
	if(is_srcC)	glBlendFunc(f1,f2);
	else		glBlendFunc(f2,f1);

	if(curr_op==op)return;
	curr_op=op;
	if(r->Support.blending_ext)
		glBlendEquation(op);

}
void TBaluRender::TBlend::Func(TVec4 blend_color,char* func){
	if(!(curr_blend_color==blend_color)&&r->Support.blending_ext)
		glBlendColor(blend_color[0],blend_color[1],blend_color[2],blend_color[3]);
	Func(func);
}

void TBaluRender::TBlend::Func(TBlendEquation left, TBlendFunc op, TBlendEquation right)
{
	glBlendEquation(blend_funcs[op]);
	glBlendFunc(blend_equations[left], blend_equations[right]);
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

void TBaluRender::TAlphaTest::Func(TAlphaTestFunc func, float val)
{
	glAlphaFunc(alpha_test_funcs[func], val);
}