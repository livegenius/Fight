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

#endif /* ALLOCATION_H_GUARD */
