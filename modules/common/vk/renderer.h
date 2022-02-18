#ifndef VK_CONTEXT_H_GUARD
#define VK_CONTEXT_H_GUARD

#include <unordered_map>
#include <SDL.h>
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>
#include "allocation.h"
#include "pipeline_builder.h"

constexpr int internalWidth = 480;
constexpr int internalHeight = 270;

class Renderer
{
	static constexpr int externalDrawThreads = 1;
public:
	static constexpr size_t bufferedFrames = 2;
	static size_t uniformBufferAligment;

	enum TextureType{
		png,
		lzs3,
	};

	struct LoadTextureInfo{
		std::filesystem::path path;
		TextureType type;
		bool repeat = false;
		bool linearFilter = false;
		bool rectangle = false;
	};

	enum BufferFlags{
		VertexStatic,
		TransferSrc, //Must provide oldBuffer.
	};

	struct Texture{
		AllocatedImage buf;
		vk::raii::ImageView view;
		vk::Extent3D extent;
		vk::Format format;
	};

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

	vk::raii::DescriptorPool descriptorPool = nullptr;

	vk::raii::CommandPool commandPool {nullptr};
	std::vector<vk::raii::CommandBuffer> cmds;

	std::vector<vk::raii::CommandPool> secondaryCommandPools;
	std::vector<vk::raii::CommandBuffer> secondaryCmds[externalDrawThreads];
	
	//Per frame data.
	struct PerFrameData{
		vk::raii::Semaphore imageAvailableSemaphore = nullptr;
		vk::raii::Semaphore renderFinishedSemaphore = nullptr;
		vk::raii::Fence inFlightFence = nullptr;
	};
	std::array<PerFrameData, bufferedFrames> frames;
	size_t currentFrame = 0;

	struct Buffer{
		AllocatedBuffer buf;
		BufferFlags flags;
		bool locked = false;
	};
	vk::Pipeline lastPipeline = VK_NULL_HANDLE;
	uint32_t imageIndex;

private:
	void RecreateSwapchain();
	void CreateSwapchain();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateDescriptorPools();
	void CreateSyncStructs();
	void BeginDrawing(int imageIndex);
	void EndDrawing(int imageIndex);
	

	void ExecuteCommand(std::function<void(vk::CommandBuffer)>&& function);

public:
	//Renderer();
	~Renderer();
	bool Init(SDL_Window *, int syncMode);
	
	bool Acquire();
	void Submit();
	bool HandleEvents(SDL_Event);
	
	void LoadTextures(const std::vector<LoadTextureInfo>& infos, std::vector<Texture> &textures);
	AllocatedBuffer NewBuffer(size_t size, BufferFlags);
	void TransferBuffer(AllocatedBuffer &src, AllocatedBuffer &dst, size_t size);
	PipelineBuilder GetPipelineBuilder();
	std::pair<vk::raii::Pipeline, vk::raii::PipelineLayout> RegisterPipelines(vk::GraphicsPipelineCreateInfo&, vk::PipelineLayoutCreateInfo&);
	std::vector<vk::DescriptorSet> CreateDescriptorSets(const std::vector<vk::DescriptorSetLayout>&);
	vk::raii::Sampler CreateSampler(vk::Filter mag = vk::Filter::eNearest, /* vk::Filter min = vk::Filter::eNearest, */ 
		vk::SamplerAddressMode mode = vk::SamplerAddressMode::eRepeat);

	const vk::CommandBuffer &GetCommand();

	void Wait();
};

#endif /* VK_CONTEXT_H_GUARD */
