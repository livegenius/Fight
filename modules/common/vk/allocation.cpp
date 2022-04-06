#include "allocation.h"

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
AllocatedBuffer::AllocatedBuffer(const vma::Allocator &allocator, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage)
{
	Allocate(allocator, size, bufferUsage, memUsage);
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer&& old)
{
	buffer = std::move(old.buffer);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	old.allocator = nullptr;
}

AllocatedBuffer& AllocatedBuffer::operator=(AllocatedBuffer&& old)
{
	buffer = std::move(old.buffer);
	allocation = std::move(old.allocation);
	allocator = old.allocator;
	old.allocator = nullptr;
	return *this;
}

void AllocatedBuffer::Allocate(const vma::Allocator &allocator_, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage)
{
	vk::BufferCreateInfo bufferInfo = {
		.size = size,
		.usage = bufferUsage,
	};

	vma::AllocationCreateInfo allocInfo = {};
	allocInfo.usage = memUsage;

	Destroy();
	allocator = &allocator_;
	auto result = allocator->createBuffer(&bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	assert(result == vk::Result::eSuccess && buffer && allocation);
}

void AllocatedBuffer::Destroy()
{
	if(allocator)
	{
		allocator->destroyBuffer(buffer, allocation);
		allocator = nullptr;
	}
}

AllocatedBuffer::~AllocatedBuffer()
{
	Destroy();
}

void* AllocatedBuffer::Map()
{
	void *mappedMem;
	allocator->mapMemory(allocation, &mappedMem);
	return mappedMem;
}

void AllocatedBuffer::Unmap()
{
	allocator->unmapMemory(allocation);
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