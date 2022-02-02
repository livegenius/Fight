#ifndef VK_CONTEXT_H_GUARD
#define VK_CONTEXT_H_GUARD

#include "../renderer.h"
#include <SDL.h>
#include <vulkan/vulkan_raii.hpp>
#include "allocation.h"

class VulkanRenderer : public Renderer
{
	vk::raii::Context context;
	vk::raii::Instance instance {nullptr};
	vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
	vk::raii::SurfaceKHR surface {nullptr};

	vk::raii::PhysicalDevice pDevice  {nullptr};
	vk::PhysicalDeviceProperties deviceProperties;
	
	vk::raii::Device device {nullptr};
	vk::Queue qGraphics;
	vk::Queue qPresent;
	struct{
		unsigned int graphics;
		unsigned int presentation;
	}indices;
	
	vma::AllocatorEx allocator; //Must be destroyed before device.

public:
	//VulkanRenderer();
	~VulkanRenderer();

	bool Init(SDL_Window *, int syncMode);
	void Submit();
	bool HandleEvents(SDL_Event);

	static size_t uniformBufferAligment;
};

#endif /* VK_CONTEXT_H_GUARD */
