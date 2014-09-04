#include "../../Include/baluRender.h"

#include "../baluRenderCommon.h"

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

TTextureId TBaluRender::TTexture::Create(const char* fname)
{
	sprintf_s(r->log_buff, "Texture loading (%s)...", fname); r->log_file.Write(r->log_buff);
	TTextureId result;
	TImage img;

	try{
		img.Load(fname);
	}catch(const char* str){
		MessageBoxA(NULL,str,"Image load error!",MB_OK|MB_ICONERROR|MB_APPLMODAL);
		return TTextureId();
	}

	GLuint target=GL_TEXTURE_2D;
	GLuint format=formats[img.GetFormat()];

	int width=img.GetWidth();
	int height=img.GetHeight();


	glGenTextures(1,(GLuint*)&result.id);

	if(r->textures.size()<=result.id)r->textures.resize(result.id+1);
	TTextureDesc& desc=r->textures[result.id];

	glBindTexture(target,result.id);

	glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	if(format==GL_COMPRESSED_RGBA_S3TC_DXT1_EXT||
		format==GL_COMPRESSED_RGBA_S3TC_DXT3_EXT||
		format==GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		//assert(GLEW_GL_ARB_texture_compression);
		int	w    = width;
		int	h    = height;
		int	offs = 0;
		int blockSize;
		int	size;
		if( format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
			blockSize = 8;
		else
			blockSize = 16;
		for ( int i = 0; i < img.GetMipMapCount(); i++ )
		{
			if ( w  == 0 )w  = 1;
			if ( h == 0 )h = 1;
			size = ((w+3)/4) * ((h+3)/4) * blockSize;
			//TODO если не поддерживается аппаратное сжатие разжимаем в обычную текстуру
			glCompressedTexImage2DARB ( target, i, format, w, h, 0, size, (void*)((int)img.GetPixels() + offs) );
			offs += size;
			w = w / 2;
			h = h / 2;
		}
	}
	else
	{
		if(r->Support.hw_generate_mipmap)
		{
			glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);//TODO добавить поддержку glGenerateMipmap из EXT_framebuffer_object
			glTexImage2D(target,0,format,width,height,0,format,GL_UNSIGNED_BYTE,img.GetPixels());
		}else{
			gluBuild2DMipmaps(
				target,format,img.GetWidth(),img.GetHeight(),
				format,GL_UNSIGNED_BYTE,img.GetPixels());
		}
	};

	glTexParameteri(target,GL_TEXTURE_MIN_FILTER,tex_filters[TTexFilter::Bilinear][min_filter]);
	glTexParameteri(target,GL_TEXTURE_MAG_FILTER,tex_filters[TTexFilter::Bilinear][mag_filter]);

	desc.clamp = TTexClamp::NONE ;
	desc.type = TTexType::TEX_2D ;
	desc.height = height;
	desc.width = width;
	desc.format = img.GetFormat();
	desc.used  = true;
	desc.filter = TTexFilter::Bilinear;
	
	sprintf_s(r->log_buff, " passed\n"); r->log_file.Write(r->log_buff);
	return result;
}

TTextureId TBaluRender::TTexture::Create(TTexType::Enum use_type, TFormat::Enum use_format,
			int use_width, int use_height, TTexFilter::Enum use_filter)
{
	TTextureId result;
	GLuint tex_id;
	GLuint target=targets[use_type];
	GLuint format=formats[use_format];
	int width=use_width;
	int height=use_height;

	glGenTextures(1,&tex_id);
	glBindTexture(target,tex_id);

	if(r->Support.hw_generate_mipmap&&use_filter>1)
		glTexParameteri(target,GL_GENERATE_MIPMAP_SGIS,GL_TRUE);

	switch(target)
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

void TBaluRender::TTexture::SetFilter(TTextureId use_tex,TTexFilter::Enum use_filter, 
			TTexClamp::Enum use_clamp, int use_aniso)
{
	TTextureDesc& desc=r->textures[use_tex.id];
	glBindTexture(targets[desc.type],use_tex.id);
	glTexParameteri(targets[desc.type],GL_TEXTURE_MIN_FILTER,tex_filters[use_filter][min_filter]);
	glTexParameteri(targets[desc.type],GL_TEXTURE_MAG_FILTER,tex_filters[use_filter][mag_filter]);
	CheckGLError();
	if((use_clamp&TTexClamp::S))
		glTexParameteri(targets[desc.type],GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	if((use_clamp&TTexClamp::T)&&desc.type!=TTexType::TEX_1D)
		glTexParameteri(targets[desc.type],GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	if((use_clamp&TTexClamp::R)&&desc.type==TTexType::TEX_3D)
		glTexParameteri(targets[desc.type],GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
	CheckGLError();
	if(use_filter>=TTexFilter::BilinearAniso&&desc.type==TTexType::TEX_2D
		&&r->Support.anisotropic_filter)
	{
		glTexParameteri(targets[desc.type], GL_TEXTURE_MAX_ANISOTROPY_EXT, use_aniso <= r->p->max_aniso ? (use_aniso>0 ? use_aniso : 1) : r->p->max_aniso);
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

	if(r->textures.size()<=result.id)r->textures.resize(result.id+1);
	TTextureDesc& desc=r->textures[result.id];

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
