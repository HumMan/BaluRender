#include "../../Include/baluRender.h"

const GLuint vert_buf_types[]=
{
	GL_ARRAY_BUFFER_ARB,
	GL_ELEMENT_ARRAY_BUFFER_ARB
};

const GLuint vert_buf_access[]=
{
	GL_READ_ONLY,
	GL_WRITE_ONLY,
	GL_READ_WRITE,
};

const GLuint vert_buf_usage[]=
{
	GL_STREAM_DRAW_ARB,
	GL_STREAM_READ_ARB,
	GL_STREAM_COPY_ARB,
	GL_STATIC_DRAW_ARB,
	GL_STATIC_READ_ARB,
	GL_STATIC_COPY_ARB,
	GL_DYNAMIC_DRAW_ARB,
	GL_DYNAMIC_READ_ARB,
	GL_DYNAMIC_COPY_ARB
};

TVertexBufferId TBaluRender::TVertexBuffer::Create(
	TVBType::Enum use_type, 
	int use_size, 
	TVBRefresh::Enum use_refresh, 
	TVBUsage::Enum use_usage)
{
	TVertexBufferId result;
	if(r->Support.vertex_buffer)
	{
		GLuint buff_id;
		GLuint usage=vert_buf_usage[use_refresh*3+use_usage];
		GLuint buff_type=vert_buf_types[use_type];

		glGenBuffersARB(1,&buff_id);

		if(r->vertex_buffers.size()<=(int)buff_id)r->vertex_buffers.resize(buff_id+1);
		TVertexBufferDesc& desc=r->vertex_buffers[buff_id];
		desc.buff_type = use_type;
		desc.buff_usage=use_usage;

		glBindBufferARB(buff_type,buff_id);
		glBufferDataARB(buff_type,use_size,0,usage);
		glBindBufferARB(buff_type,0);
		result.id=buff_id;
		return result;
	}else
	{
		int buff_id=r->vertex_buffers_emul.New();
		TVertexBufferDesc& desc=r->vertex_buffers_emul[buff_id];
		desc.buff_type = use_type;
		desc.data_pointer = new char[use_size];
		result.id=buff_id;
		return result;
	}
}

void* TBaluRender::TVertexBuffer::Map(TVertexBufferId use_id, TVBAccess::Enum use_access)
{
	if(r->Support.vertex_buffer)
	{
		TVertexBufferDesc& desc=r->vertex_buffers[use_id.id];
		GLuint buff_type=vert_buf_types[desc.buff_type];
		glBindBufferARB(buff_type,use_id.id);
		void* result=glMapBufferARB(buff_type,vert_buf_access[use_access]);
		CheckGLError();
		glBindBufferARB(buff_type,0);
		return result;
	}else
	{
		TVertexBufferDesc& desc=r->vertex_buffers_emul[use_id.id];
		return desc.data_pointer;
	}
}

void TBaluRender::TVertexBuffer::Unmap(TVertexBufferId use_id)
{
	if(r->Support.vertex_buffer)
	{
		TVertexBufferDesc& desc=r->vertex_buffers[use_id.id];
		GLuint buff_type=vert_buf_types[desc.buff_type];
		glBindBufferARB(buff_type,use_id.id);
		glUnmapBufferARB(buff_type);
		glBindBufferARB(buff_type,0);
	}else
	{
	}
}

void TBaluRender::TVertexBuffer::SubData(TVertexBufferId use_id,int use_offset,int use_size,void* use_new_data)
{
	if(r->Support.vertex_buffer)
	{
		TVertexBufferDesc& desc=r->vertex_buffers[use_id.id];
		GLuint buff_type=vert_buf_types[desc.buff_type];
		glBindBufferARB(buff_type,use_id.id);
		CheckGLError();
		glBufferSubDataARB(buff_type,use_offset,use_size,use_new_data);
		CheckGLError();
		glBindBufferARB(buff_type,0);
		CheckGLError();
	}else
	{
		TVertexBufferDesc& desc=r->vertex_buffers_emul[use_id.id];
		memcpy((char*)desc.data_pointer+use_offset,use_new_data,use_size);
	}
}

void TBaluRender::TVertexBuffer::Data(TVertexBufferId use_id,int use_size,void* use_new_data)
{
	if(r->Support.vertex_buffer)
	{
		TVertexBufferDesc& desc=r->vertex_buffers[use_id.id];
		GLuint buff_type=vert_buf_types[desc.buff_type];
		glBindBufferARB(buff_type,use_id.id);
		glBufferDataARB(buff_type,use_size,use_new_data,vert_buf_usage[desc.buff_usage]);
		CheckGLError();
		glBindBufferARB(buff_type,0);
		CheckGLError();
	}else
	{
		TVertexBufferDesc& desc=r->vertex_buffers_emul[use_id.id];
		memcpy((char*)desc.data_pointer,use_new_data,use_size);
	}
}

void TBaluRender::TVertexBuffer::Delete(TVertexBufferId use_id)
{
	if(r->Support.vertex_buffer)
	{
		glDeleteBuffersARB(1,(GLuint*)&use_id.id);
	}else if(use_id.id!=0)//TODO решить что делать с этим(или ошибка или ничего не делать)
	{
		delete r->vertex_buffers_emul[use_id.id].data_pointer;
		r->vertex_buffers_emul.Free(use_id.id);
	}
}
