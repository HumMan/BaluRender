#include "../../Include/baluRender.h"

#include "../baluRenderCommon.h"

const GLuint attach_types[]=
{
	GL_COLOR_ATTACHMENT0_EXT,
	GL_COLOR_ATTACHMENT1_EXT,
	GL_COLOR_ATTACHMENT2_EXT,
	GL_COLOR_ATTACHMENT3_EXT,
	GL_DEPTH_ATTACHMENT_EXT
};

int CreateRenderBuffer(TFormat::Enum format,int width,int height)
{
	//TODO вроде проверяется ARB а не EXT
	GLuint r=0;
	glGenRenderbuffersEXT(1,&r);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,r);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,formats[format],width,height);
	return r;
}

int ResizeRenderBuffer(GLuint render_buf,TFormat::Enum format,int width,int height)
{
	int r=0;
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,render_buf);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,formats[format],width,height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	return r;
}

void TBaluRender::TFrameBuffer::CheckStatus(TFrameBufferId use_framebuf_id)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,use_framebuf_id.id);
	unsigned int s=glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	switch(s)
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		return;
		break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      throw "Incomplete attachment";
	  break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      throw "Missing attachment";
	  break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      throw "Incomplete dimensions";
	  break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      throw "Incomplete formats";
	  break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      throw "Incomplete draw buffer";
	  break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      throw "Incomplete read buffer";
	  break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
      throw "Framebuffer unsupported";
	  break;
	}
}

TFrameBufferId TBaluRender::TFrameBuffer::Create(int use_width,int use_height,bool use_color,bool use_depth)
{
	GLuint frame_buffer_id;
	glGenFramebuffersEXT(1,&frame_buffer_id);

	TFrameBufferDesc& desc=r->frame_buffers[frame_buffer_id];
	desc.used = true;
	desc.width = use_width;
	desc.height = use_height;
	desc.use_color=use_color;
	desc.use_depth=use_depth;
	desc.color_buffer_id = 0;
	desc.depth_buffer_id = 0;
	desc.current_render_face = 0;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,frame_buffer_id);
	if (use_depth)
	{
		desc.depth_buffer_id=CreateRenderBuffer(TFormat::DEPTH24 ,desc.width,desc.height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
			GL_RENDERBUFFER_EXT,desc.depth_buffer_id);
	}

	if (use_color)
	{
		desc.color_buffer_id=CreateRenderBuffer(TFormat::RGBA8,desc.width,desc.height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,
			GL_RENDERBUFFER_EXT,desc.color_buffer_id);
	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	TFrameBufferId result;
	result.id = frame_buffer_id;
	return result;
}

void TBaluRender::TFrameBuffer::Delete(TFrameBufferId use_framebuff)
{
	TFrameBufferDesc& desc=r->frame_buffers[use_framebuff.id];
	desc.used=false;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	glDeleteFramebuffersEXT(1,(GLuint*)&use_framebuff.id);
	if(desc.use_depth)
		glDeleteRenderbuffersEXT(1,(GLuint*)&desc.depth_buffer_id);
	if(desc.use_color)
		glDeleteRenderbuffersEXT(1,(GLuint*)&desc.color_buffer_id);
}

void TBaluRender::TFrameBuffer::Resize(TFrameBufferId use_framebuff, int use_width, int use_height)
{
	TFrameBufferDesc& desc=r->frame_buffers[use_framebuff.id];

	desc.width=use_width;
	desc.height=use_height;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

	if (desc.use_depth)
		ResizeRenderBuffer(desc.depth_buffer_id,TFormat::DEPTH24,use_width,use_height);

	if (desc.use_color)
		ResizeRenderBuffer(desc.color_buffer_id,TFormat::RGBA8,use_width,use_height);
}

void TBaluRender::TFrameBuffer::AttachTexture(TFrameBufferId use_framebuff, int use_draw_buff_index,TTextureId use_texture,int use_cube_side)
{
	TFrameBufferDesc& desc=r->frame_buffers[use_framebuff.id];
	TTextureDesc& tex_desc=r->textures[use_texture.id];

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,use_framebuff.id);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,attach_types[use_draw_buff_index],//TODO check use_draw_buff_index
		tex_desc.type==TTexType::CUBE ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + use_cube_side):GL_TEXTURE_2D,
		use_texture.id,0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
}

void TBaluRender::TFrameBuffer::AttachDepthTexture(TFrameBufferId use_framebuff, TTextureId use_texture,int use_cube_side)
{
	TFrameBufferDesc& desc=r->frame_buffers[use_framebuff.id];
	TTextureDesc& tex_desc=r->textures[use_texture.id];

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,use_framebuff.id);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,
		tex_desc.type==TTexType::CUBE ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + use_cube_side):GL_TEXTURE_2D,
		use_texture.id,0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
}

void TBaluRender::TFrameBuffer::Bind(TFrameBufferId use_framebuff)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,use_framebuff.id);
}

void TBaluRender::TFrameBuffer::BindMain()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
}

void TBaluRender::TFrameBuffer::SetDrawBuffersCount(int use_count)//TODO должно задаваться автоматически в зависимости от подсоединенных буферов цвета
{
	glDrawBuffers(use_count,&attach_types[0]);
}