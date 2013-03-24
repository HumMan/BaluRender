#include "../../Include/baluRender.h"

const GLenum asm_shader_type[]=
{
	GL_VERTEX_PROGRAM_ARB,
	GL_FRAGMENT_PROGRAM_ARB
};

TShaderId TBaluRender::TShader::Create(const char* use_file_name,const char* use_defines)
{
	return TShaderId();
}

void TBaluRender::TShader::LoadShaderASM(const int& id, const TShaderType::Enum use_type,const char* use_source,const char* use_defines)
{
	GLenum type = asm_shader_type[use_type];
	glGenProgramsARB ( 1, (GLenum*)&id );
	glBindProgramARB(type,id);
    glProgramStringARB ( type, GL_PROGRAM_FORMAT_ASCII_ARB,
                         strlen(use_source),use_source);
    if ( glGetError () == GL_INVALID_OPERATION )
    {
		char s[100];
		sprintf_s(s,100,"%s shader compile error",use_type==0?"Vertex":"Fragment");
        MessageBoxA(0,(char*)glGetString ( GL_PROGRAM_ERROR_STRING_ARB ),s,MB_OK);
		exit(0);
    }
	glBindProgramARB(type,0);
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
		MessageBoxA(0,buf,"Shader compile error:",MB_OK);
		MessageBoxA(0,use_source,"Source:",MB_OK);
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
	GLint linked;
	glGetObjectParameterivARB(program_object,GL_OBJECT_LINK_STATUS_ARB,&linked);
	if(!linked)
	{
		int chars_written;
		char buf[2048];
		glGetInfoLogARB(program_object,sizeof(buf),&chars_written,buf);
		MessageBoxA(0,buf,"Shader link error:",MB_OK);
	}
	TShaderId result;
	result.id = program_object;
	return result;
}

TShaderId TBaluRender::TShader::LoadASM(const char* use_vsource,const char* use_fsource,const char* use_defines)
{
	return TShaderId();
}

void TBaluRender::TShader::Delete(const TShaderId use_shader)
{
}

void TBaluRender::TShader::Bind(const TShaderId use_shader)
{
}

void TBaluRender::TShader::Unbind(const TShaderId use_shader)
{
}

int TBaluRender::TShader::GetUniformLocation(const TShaderId use_shader, const char* use_uniform_name)
{
	return -1;
}

void TBaluRender::TShader::SetUniform(const TShaderId use_shader, int location,const TVec3& use_vec)
{
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
