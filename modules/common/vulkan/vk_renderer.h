#ifndef VK_CONTEXT_H_GUARD
#define VK_CONTEXT_H_GUARD

#include "../renderer.h"
#include <unordered_map>
#include <SDL.h>
#include <vulkan/vulkan_raii.hpp>
#include "allocation.h"

class VulkanRenderer : public Renderer
{
public:
	static constexpr size_t bufferedFrames = 2;
	static size_t uniformBufferAligment;

private:
	SDL_Window *window = nullptr;
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

	struct {
		vk::raii::SwapchainKHR handle = {nullptr};
		std::vector<VkImage> images;
		std::vector<vk::raii::ImageView> imageViews;
		vk::Extent2D extent;
		vk::Format format;
		std::vector<vk::raii::Framebuffer> framebuffers;

		std::vector<AllocatedImage> depthImages;
		std::vector<vk::raii::ImageView> depthImageViews; //Maybe multiple?
	} swapchain;
	vk::raii::RenderPass renderPass {nullptr};

	bool shouldDraw;
	float aspectRatio;
	vk::Viewport viewportArea {0,0, 0,0, 0.0,1.f};
	vk::Rect2D scissorArea;

	struct {
		vk::raii::Fence fence = nullptr;
		vk::raii::CommandPool cmdPool = nullptr;
		vk::raii::CommandBuffer cmd = nullptr;
	} upload;

	vk::raii::CommandPool commandPool {nullptr};
	std::vector<vk::raii::CommandBuffer> cmds;
	
	//Per frame data.
	struct PerFrameData{
		vk::raii::Semaphore imageAvailableSemaphore = nullptr;
		vk::raii::Semaphore renderFinishedSemaphore = nullptr;
		vk::raii::Fence inFlightFence = nullptr;
	};
	std::array<PerFrameData, bufferedFrames> frames;
	size_t currentFrame = 0;

	size_t mapCounter = 0;

	struct Texture{
		AllocatedImage buf;
		vk::raii::ImageView view;
		vk::Extent3D extent;
		vk::Format format;
		AllocatedBuffer* stagingBufferRef;
	};
	std::unordered_map<size_t, Texture> textures;

private:
	void RecreateSwapchain();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateSyncStructs();
	void Draw(int imageIndex);

	void ExecuteCommand(std::function<void(vk::CommandBuffer)>&& function);

public:
	//VulkanRenderer();
	~VulkanRenderer();

	bool Init(SDL_Window *, int syncMode);
	void Submit();
	bool HandleEvents(SDL_Event);

	std::vector<int> LoadTextures(std::vector<LoadTextureInfo>&);
	
};

#endif /* VK_CONTEXT_H_GUARD */
