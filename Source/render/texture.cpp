#include "../../Include/baluRender.h"

using namespace BaluRender;

#include "../baluRenderCommon.h"

#include <IL/ilut.h>

#include <easylogging++.h>

using namespace TBaluRenderEnums;

const bool mag_filter=0;      
const bool min_filter=1;
const GLuint tex_filters[][2]=
{
	{GL_NEAREST,   GL_NEAREST},
	{GL_LINEAR ,   GL_NEAREST},
	{GL_LINEAR,    GL_LINEAR_MIPMAP_NEAREST},
	{GL_LINEAR,    GL_LINEAR_MIPMAP_LINEAR },
	{GL_LINEAR,    GL_LINEAR_MIPMAP_NEAREST},
	{GL_LINEAR,    GL_LINEAR_MIPMAP_LINEAR }
};

void HandleDevILErrors()
{
	ILenum error = ilGetError();

	if (error != IL_NO_ERROR) {
		do {
			const char* ch = iluErrorString(error);
			printf("\n\n%s\n", ch);
			LOG(ERROR) << ch;
		} while ((error = ilGetError()));
	}
}

TTextureId TBaluRender::TTexture::Create(const char* fname)
{
	LOG(INFO) << "Start create image: "<< fname;
	//ilutRenderer(ILUT_OPENGL);
	ILuint ImgId;
	ilGenImages(1, &ImgId);
	ilBindImage(ImgId);

	if (!ilLoadImage(fname))
	{
		HandleDevILErrors();
		return TTextureId();
	}
	else
	{
		GLuint id;
		// Goes through all steps of sending the image to OpenGL.
		id = ilutGLBindTexImage();
		ilutGLBuildMipmaps();

		// We're done with our image, so we go ahead and delete it.
		ilDeleteImages(1, &ImgId);

		HandleDevILErrors();

		TTextureId result = *(TTextureId*)&id;

		GLuint target = GL_TEXTURE_2D;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(target, id);

		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		LOG(INFO) << "End create image";
		return result;
	}
}

TTextureId TBaluRender::TTexture::Create(TTexType use_type, TFormat use_format,
			int use_width, int use_height, TTexFilter use_filter)
{
	TTextureId result;
	GLuint tex_id;
	GLuint target = targets[(int)use_type];
	GLuint format = formats[(int)use_format];
	int width=use_width;
	int height=use_height;

	glGenTextures(1,&tex_id);
	glBindTexture(target,tex_id);

	if (r->Support.hw_generate_mipmap && (int)use_filter>1)
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);

	switch((TTexType)target)
	{
	case TTexType::TEX_1D:
		glCopyTexImage1D(target,0,format,0,0,use_width,0);
		break;
	case TTexType::TEX_2D:
		glCopyTexImage2D(target,0,format,0,0,width,height,0);
		break;
	//case TTexType::TEX_3D:
	//	glCopyTexSubImage3D(target,0,format,0,0,use_width,0);
	//	break;
	case TTexType::CUBE:
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		for(int i=0;i<6;i++)
			glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,format,0,0,width,height,0);
		break;
	default:assert(false);
	};
	result.id=tex_id;
	return result;
}

void TBaluRender::TTexture::Delete(TTextureId use_tex)
{
	glDeleteTextures(1,(GLuint*)&use_tex.id);
}

void TBaluRender::TTexture::SetFilter(TTextureId use_tex,TTexFilter use_filter, 
			TTexClamp use_clamp, int use_aniso)
{
	TTextureDesc& desc=r->p->textures[use_tex.id];
	glBindTexture(targets[(int)desc.type], use_tex.id);
	glTexParameteri(targets[(int)desc.type], GL_TEXTURE_MIN_FILTER, tex_filters[(int)use_filter][min_filter]);
	glTexParameteri(targets[(int)desc.type], GL_TEXTURE_MAG_FILTER, tex_filters[(int)use_filter][mag_filter]);
	CheckGLError();
	if (((int)use_clamp&(int)TTexClamp::S))
		glTexParameteri(targets[(int)desc.type], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	if (((int)use_clamp&(int)TTexClamp::T) && desc.type != TTexType::TEX_1D)
		glTexParameteri(targets[(int)desc.type], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (((int)use_clamp&(int)TTexClamp::R) && desc.type == TTexType::TEX_3D)
		glTexParameteri(targets[(int)desc.type], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	CheckGLError();
	if(use_filter>=TTexFilter::BilinearAniso&&desc.type==TTexType::TEX_2D
		&&r->Support.anisotropic_filter)
	{
		glTexParameteri(targets[(int)desc.type], GL_TEXTURE_MAX_ANISOTROPY_EXT, use_aniso <= r->p->max_aniso ? (use_aniso>0 ? use_aniso : 1) : r->p->max_aniso);
	}
	CheckGLError();
}

TTextureId TBaluRender::TTexture::CreateTarget()
{
	TTextureId result;

	GLuint target=GL_TEXTURE_2D;
	GLuint format=GL_RGBA;

	TVec2i screen_size=r->ScreenSize();

	if(!r->Support.npot_texture)
	{
		screen_size[0]=GetGreaterPOT(screen_size[0]);
		screen_size[1]=GetGreaterPOT(screen_size[1]);
	}

	glGenTextures(1,(GLuint*)&result.id);

	if(r->p->textures.size()<=result.id)r->p->textures.resize(result.id+1);
	TTextureDesc& desc=r->p->textures[result.id];

	glBindTexture(target,result.id);

	if(r->Support.hw_generate_mipmap)
	{
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);
	}else
	{
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}
	CheckGLError();
	glCopyTexImage2D(target,0,format,0,0,screen_size[0],screen_size[1],0);
	CheckGLError();
	
	desc.clamp = TTexClamp::NONE ;
	desc.type = TTexType::TEX_2D ;
	desc.width = screen_size[0];
	desc.height = screen_size[1];
	desc.format = TFormat::RGBA8;
	desc.used  = true;
	desc.filter = TTexFilter::Bilinear;

	return result;
}

void TBaluRender::TTexture::Enable(bool enable)
{
	if(enable)glEnable(GL_TEXTURE_2D);
	else glDisable(GL_TEXTURE_2D);
}
void TBaluRender::TTexture::Bind(TTextureId use_tex)
{
	glBindTexture(GL_TEXTURE_2D,use_tex.id);
}
void TBaluRender::TTexture::CopyFramebufferTo(TTextureId use_tex)
{
	glBindTexture(GL_TEXTURE_2D,use_tex.id);
	TVec2i screen_size=r->ScreenSize();
	if(r->Support.npot_texture)
		glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,screen_size[0],screen_size[1],0);
	else
		glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,GetGreaterPOT(screen_size[0]),GetGreaterPOT(screen_size[1]),0);
}
TVec2 TBaluRender::TTexture::GetRTCoords()
{
	//assert(is render target)
	TVec2i screen_size=r->ScreenSize();
	if(r->Support.npot_texture) 
		return TVec2(1.0,1.0);
	else return TVec2(
		float(screen_size[0])/GetGreaterPOT(screen_size[0]),
		float(screen_size[1])/GetGreaterPOT(screen_size[1]));
}
