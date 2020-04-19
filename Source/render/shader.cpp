#include "../../Include/baluRender.h"

using namespace BaluRender;

#include "../baluRenderCommon.h"

using namespace TBaluRenderEnums;

#include <string.h>

#if defined(WIN32)||defined(_WIN32)
#else
#define sprintf_s sprintf
#define sscanf_s sscanf
#endif

const GLenum asm_shader_type[]=
{
	GL_VERTEX_PROGRAM_ARB,
	GL_FRAGMENT_PROGRAM_ARB
};

TShaderId TBaluRender::TShader::Create(const char* use_file_name,const char* use_defines)
{
	return TShaderId();
}

void TBaluRender::TShader::LoadShaderGLSL(const int use_shader,const char* use_source,const char* use_defines)
{
	const GLcharARB* shader_strings[2];
	int len[2];
	shader_strings[0]=use_defines;
	shader_strings[1]=use_source;
	len[0]=strlen(use_defines);
	len[1]=strlen(use_source);
	glShaderSourceARB(use_shader,2,shader_strings,len);
	glCompileShaderARB(use_shader);
	CheckGLError();
	GLint status;
	glGetObjectParameterivARB(use_shader,GL_OBJECT_COMPILE_STATUS_ARB,&status);
	if(!status)
	{
		int chars_written;
		char buf[2048];
		glGetInfoLogARB(use_shader,sizeof(buf),&chars_written,buf);
	}
}

TShaderId TBaluRender::TShader::LoadGLSL(const char* use_vsource,const char* use_fsource,const char* use_defines)
{
	GLhandleARB program_object=glCreateProgramObjectARB();
	CheckGLError();
	GLhandleARB fshader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	GLhandleARB vshader=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	LoadShaderGLSL(fshader,use_fsource,use_defines);
	LoadShaderGLSL(vshader,use_vsource,use_defines);
	glAttachObjectARB(program_object,vshader);
	glAttachObjectARB(program_object,fshader);
	glLinkProgramARB(program_object);

	glDetachObjectARB(program_object, vshader);
	glDetachObjectARB(program_object, fshader);

	glDeleteObjectARB(vshader);
	glDeleteObjectARB(fshader);

	GLint linked;
	glGetObjectParameterivARB(program_object,GL_OBJECT_LINK_STATUS_ARB,&linked);
	if(!linked)
	{
		int chars_written;
		char buf[2048];
		glGetInfoLogARB(program_object,sizeof(buf),&chars_written,buf);
	}

	GLint uniforms_count;
	glGetProgramivARB(program_object, GL_ACTIVE_UNIFORMS, &uniforms_count);
	for (int i = 0; i < uniforms_count; i++)
	{
		int length, size;
		char name[256];
		GLenum type;
		glGetActiveUniformARB(program_object, i, 256, &length, &size, &type, name);
	}

	TShaderId result;
	result.id = program_object;
	return result;
}

void TBaluRender::TShader::Delete(const TShaderId use_shader)
{
	glUseProgramObjectARB(0);
	glDeleteProgram(use_shader.id);
}

void TBaluRender::TShader::Bind(const TShaderId use_shader)
{
	glUseProgramObjectARB(use_shader.id);
}

void TBaluRender::TShader::Unbind(const TShaderId use_shader)
{
	glUseProgramObjectARB(0);
}

int TBaluRender::TShader::GetUniformLocation(const TShaderId use_shader, const char* use_uniform_name)
{
	return glGetUniformLocationARB(use_shader.id, use_uniform_name);
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,const TVec3& use_vec)
{
	//GLint id;
	//glGetIntegerv(GL_CURRENT_PROGRAM, &id);
	//TODO assert(active_program==use_shader);
	glUniform3fvARB(location, 1, (GLfloat*) &use_vec);
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,const TVec4& use_vec)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,const TMatrix3& use_mat)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,const TMatrix4& use_mat)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,int count,const TVec3& use_vec)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,int count,const TVec4& use_vec)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,int count,const TMatrix3& use_mat)
{
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,int count,const TMatrix4& use_mat)
{
}

//void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location, int count, void* value)
//{
//
//}
