#include "vertex_buffer.h"
#include <iostream>
#include <cstring>
#include "renderer.h"

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
		totalSize/stride, //TODO: Take a better look at this. Really really do?
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

void VertexBuffer::UpdateBuffer(int which, void *src, size_t count, int index)
{
	if(count == 0)
		count = dataPointers[which].size;

	uint8_t* dst = (uint8_t*)buffer.Map(index);
	memcpy(dst+dataPointers[which].location*dataPointers[which].stride, src, count);
}

void VertexBuffer::Load()
{
	buffer.Allocate(&renderer, totalSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vma::MemoryUsage::eGpuOnly);
	AllocatedBuffer stagingBuffer(&renderer, totalSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
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

void VertexBuffer::LoadHostVisible(int copies)
{
	buffer.Allocate(&renderer, totalSize, vk::BufferUsageFlagBits::eVertexBuffer, vma::MemoryUsage::eCpuToGpu, copies);
	uint8_t *data = (uint8_t*)buffer.Map();
	size_t where = 0;
	for(auto &subData : dataPointers)
	{
		if(subData.ptr)
			for(int i = 0; i < copies; ++i)
				memcpy(data+where+buffer.copySize*i, subData.ptr, subData.size);
		where += subData.size;
	}
	buffer.Unmap();
}
