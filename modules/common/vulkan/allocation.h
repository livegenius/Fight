#ifndef ALLOCATION_H_GUARD
#define ALLOCATION_H_GUARD

#include <vk_mem_alloc.hpp>

namespace vma {
class AllocatorEx : public Allocator
{
	public:
	~AllocatorEx()
	{
		destroy();
	}
};
}

struct AllocatedImage {
	vk::Image image;
	vma::Allocation allocation;
	const vma::Allocator *allocator = nullptr;
	
	AllocatedImage() = default;
	AllocatedImage(const vma::Allocator &, const vk::ImageCreateInfo, const vma::MemoryUsage, const vk::MemoryPropertyFlags = {});
	AllocatedImage(AllocatedImage&&);
	AllocatedImage(const AllocatedImage&) = delete;
	AllocatedImage& operator=(const AllocatedImage&) = delete;
	void Allocate(const vma::Allocator &, const vk::ImageCreateInfo, const vma::MemoryUsage, const vk::MemoryPropertyFlags = {});
	void Destroy();
	~AllocatedImage();
};

struct AllocatedBuffer {
	vk::Buffer buffer;
	vma::Allocation allocation;
	const vma::Allocator *allocator = nullptr;

	AllocatedBuffer() = default;
	AllocatedBuffer(const vma::Allocator &allocator, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage);
	AllocatedBuffer(AllocatedBuffer&&);
	AllocatedBuffer(const AllocatedBuffer&) = delete;
	AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;

	void* Map();
	void Unmap();
	void CopyData(void*, size_t);
	//void CopyDataAt(void* input, size_t pos, size_t size);
	void Allocate(const vma::Allocator &allocator, vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, vma::MemoryUsage memUsage);
	void Destroy();
	~AllocatedBuffer();
};


#endif /* ALLOCATION_H_GUARD */
