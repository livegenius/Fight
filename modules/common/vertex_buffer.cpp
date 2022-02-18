#include "vertex_buffer.h"
#include <iostream>
#include <cstring>

VertexBuffer::VertexBuffer(Renderer *renderer):
renderer(*renderer)
{

}

VertexBuffer::~VertexBuffer()
{

}

int VertexBuffer::Prepare(size_t size, unsigned int stride, void *ptr)
{
	if(int alignment = size % stride)
	{
		std::cerr << __FUNCTION__ << ": Size "<<size<<" is not a multiple of the stride "<<stride<<".\n";
		size -= alignment;
	}

	dataPointers.push_back(memPtr{
		(uint8_t*) ptr,
		size,
		totalSize/stride, //TODO: Take a better look at this.
		stride,
	});
	totalSize += size;
	return dataPointers.size() - 1;
}

std::pair<size_t,size_t> VertexBuffer::Index(int which) const
{
	const auto &data = dataPointers[which];
	return{data.location, data.size/data.stride};
}

void VertexBuffer::UpdateBuffer(int which, void *data, size_t count)
{
	if(count == 0)
		count = dataPointers[which].size;

/* 	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferSubData(GL_ARRAY_BUFFER, dataPointers[which].location*stride, count, data); */
}

void VertexBuffer::UpdateElementBuffer(void *data, size_t count)
{
	if(count > eboSize)
		count = eboSize;
/* 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, count, data); */
}

void VertexBuffer::Bind()
{
/* 	glBindVertexArray(vaoId);
	if(eboId)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId); */
}

void VertexBuffer::Load()
{
	Bind();
	buffer = renderer.NewBuffer(totalSize, Renderer::BufferFlags::VertexStatic);
	AllocatedBuffer stagingBuffer = renderer.NewBuffer(totalSize, Renderer::BufferFlags::TransferSrc);
	uint8_t *data = (uint8_t*)stagingBuffer.Map();
	size_t where = 0;
	for(auto &subData : dataPointers)
	{
		if(subData.ptr)
			memcpy(data+where, subData.ptr, subData.size);
		where += subData.size;
	}

	stagingBuffer.Unmap();
	renderer.TransferBuffer(stagingBuffer, buffer, totalSize);
}
