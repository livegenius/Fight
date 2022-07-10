#include "allocation.h"
#include "renderer.h"

AllocatedImage::AllocatedImage(const vma::Allocator &allocator, const vk::ImageCreateInfo imageInfo, const vma::MemoryUsage memUsage, const vk::MemoryPropertyFlags flags)
{
	Allocate(allocator, imageInfo, memUsage, flags);
}

AllocatedImage::AllocatedImage(AllocatedImage&& old)
{
	image = std::move(old.image);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	old.allocator = nullptr;
}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& old)
{
	image = std::move(old.image);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	old.allocator = nullptr;
	return *this;
}


void AllocatedImage::Allocate(const vma::Allocator &allocator_, const vk::ImageCreateInfo imageInfo, const vma::MemoryUsage memUsage, const vk::MemoryPropertyFlags flags)
{
	vma::AllocationCreateInfo allocInfo = {};
	allocInfo.usage = memUsage;
	allocInfo.requiredFlags = flags;
	Destroy();
	allocator = &allocator_;
	vk::Result result = allocator->createImage(&imageInfo, &allocInfo, &image, &allocation, nullptr);
	assert(result == vk::Result::eSuccess && allocation && image);
}

void AllocatedImage::Destroy()
{
	if(allocator)
	{
		allocator->destroyImage(image, allocation);
		allocator = nullptr;
	}
}

AllocatedImage::~AllocatedImage()
{
	Destroy();
}

/////////////
AllocatedBuffer::AllocatedBuffer(const Renderer *renderer, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage, int copies)
{
	Allocate(renderer, size, bufferUsage, memUsage, copies);
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer&& old)
{
	buffer = std::move(old.buffer);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	data = old.data;
	copySize = old.copySize;
	copies = old.copies;
	old.allocator = nullptr;
	old.data = nullptr;
}

AllocatedBuffer& AllocatedBuffer::operator=(AllocatedBuffer&& old)
{
	buffer = std::move(old.buffer);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	old.allocator = nullptr;
	return *this;
}

void AllocatedBuffer::Allocate(const Renderer *renderer, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage, int copies_)
{
	assert(copies_ > 0);

	copies = copies_;
	copySize = Renderer::PadToAlignment(size);
	auto allocatedSize = copies * copySize;
	vk::BufferCreateInfo bufferInfo = {
		.size = allocatedSize,
		.usage = bufferUsage,
	};

	vma::AllocationCreateInfo allocInfo = {};
	allocInfo.usage = memUsage;

	Destroy();
	allocator = &renderer->GetAllocator();
	auto result = allocator->createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	assert(result == vk::Result::eSuccess && buffer && allocation);
}

void AllocatedBuffer::Destroy()
{
	if(allocator)
	{
		Unmap();
		allocator->destroyBuffer(buffer, allocation);
		allocator = nullptr;	
	}
}

AllocatedBuffer::~AllocatedBuffer()
{
	Destroy();
}

void* AllocatedBuffer::Map(int index)
{
	if(!data)
		allocator->mapMemory(allocation, &data);
	return (uint8_t*)data+copySize*index;
}

void AllocatedBuffer::Unmap()
{
	if(data)
	{
		allocator->unmapMemory(allocation);
		//allocator->flushAllocation(allocation, 0, copySize*copies);
		data = nullptr;
	}
}

void AllocatedBuffer::CopyData(void* input, size_t size)
{
	void *mappedMem;
	allocator->mapMemory(allocation, &mappedMem);
	memcpy(mappedMem, input, size);
	allocator->unmapMemory(allocation);
}

/* 
void AllocatedBuffer::CopyDataAt(void* input, size_t pos, size_t size)
{
	void *mappedMem;
	allocator->mapMemory(allocation, &mappedMem);
	uint8_t *offset = (uint8_t*)mappedMem + padToAlignment(size, uniformBufferAligment)*pos;
	memcpy(offset, input, size);
	allocator->unmapMemory(allocation);
} */