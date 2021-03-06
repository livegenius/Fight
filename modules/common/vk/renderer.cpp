#include "renderer.h"
#include <VkBootstrap.h>
#include <vulkan/vulkan_raii.hpp>
#include <SDL_vulkan.h>
#include <iostream>
#include <image.h>
#include <fstream>

size_t Renderer::uniformBufferAlignment = 0;
size_t Renderer::uniformBufferSizeLimit = 0;

bool Renderer::Init(SDL_Window *window_, int syncMode)
{
	window = window_;
	vkb::InstanceBuilder ib;
	#ifndef NDEBUG
		ib.request_validation_layers();
		ib.use_default_debug_messenger();
	#endif

	auto system_info_ret = vkb::SystemInfo::get_system_info();
	if (!system_info_ret) {
		std::cerr<<(system_info_ret.error().message());
		return false;
	}
	auto system_info = system_info_ret.value();

	uint32_t extensionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
	std::vector<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
	for(const auto& extension : extensions){
		if (system_info.is_extension_available(extension))
			ib.enable_extension(extension);
		else
		{
			std::cerr << extension << " is not available.\n";
			return false;
		}
	}

	auto result = ib.set_app_name(SDL_GetWindowTitle(window))
		.require_api_version(1, 2, 0)
		#ifndef NDEBUG
		.use_default_debug_messenger()
		#endif
		.build();
		
	if(!result)
	{
		std::cerr << result.error();
		return false;
	}
	vkb::Instance vkb_inst = result.value();
	instance = {context, vkb_inst.instance};
	debugMessenger = {instance, vkb_inst.debug_messenger};

	VkSurfaceKHR sdlSurface;
	if(!SDL_Vulkan_CreateSurface(window, *instance, &sdlSurface))
	{
		std::cerr << "Couldn't create SDL surface\n";
		return false;
	}
	surface = {instance, sdlSurface};

	//Pick and create logical device.
	vkb::PhysicalDeviceSelector phys_device_selector(vkb_inst); 
	auto phys_device_ret = phys_device_selector
			.set_surface(*surface)
			//.set_required_features(VkPhysicalDeviceFeatures{.wideLines = true})
			.require_present()
			.set_minimum_version(1, 1)
			.add_desired_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
			.select ();
	if (!phys_device_ret) {
		std::cerr <<("Couldn't select physical device.\n");
		return false;
	}
	auto phys_dev = phys_device_ret.value();
	pDevice = {instance, phys_dev.physical_device};

	deviceProperties = phys_dev.properties;
	uniformBufferAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
	uniformBufferSizeLimit = deviceProperties.limits.maxUniformBufferRange;

	vkb::DeviceBuilder device_builder(phys_dev);
	auto dev_ret = device_builder.build ();
	if (!dev_ret) {
		std::cerr <<("Couldn't create logical device.\n");
		return false;
	}

	vkb::Device vkb_device = dev_ret.value();
	device = {pDevice, vkb_device.device};
	qGraphics = vkb_device.get_queue (vkb::QueueType::graphics).value();
	qPresent = vkb_device.get_queue (vkb::QueueType::present).value();
	indices.graphics = vkb_device.get_queue_index (vkb::QueueType::graphics).value();
	indices.presentation = vkb_device.get_queue_index (vkb::QueueType::present).value();

	vma::AllocatorCreateInfo aInfo;
	aInfo.physicalDevice = *pDevice,
	aInfo.device = *device,
	aInfo.instance = *instance,
	vma::createAllocator(&aInfo, &allocator);

/* 	vk::MemoryType a;
	auto props = pDevice.getMemoryProperties();
 */
	CreateSwapchain();
	CreateRenderPass();
	CreateFramebuffers();
	CreateCommandPool();
	CreateDescriptorPools();
	CreateSyncStructs();
	return true;
}

Renderer::~Renderer()
{
	device.waitIdle();
}

void Renderer::CreateSwapchain()
{
	if(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
	{
		shouldDraw = false;
		return;
	}

	vkb::SwapchainBuilder swapchainBuilder{*pDevice, *device, *surface};
	int width, height;
	SDL_Vulkan_GetDrawableSize(window, &width, &height);
	aspectRatio = (float)width/(float)height;
	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
		.set_desired_extent(width, height)
		.set_old_swapchain(*swapchain.handle)
		.build()
		.value();
	
	viewportArea.width = width;
	viewportArea.height = height;
	scissorArea.extent.width = width;
	scissorArea.extent.height = height;

	//store swapchain and its related images
	swapchain.handle = {device, vkbSwapchain.swapchain};
	swapchain.images = vkbSwapchain.get_images().value();
	auto imageViewVector = vkbSwapchain.get_image_views().value();
	swapchain.imageViews.clear();
	for(VkImageView &view : imageViewVector)
		swapchain.imageViews.push_back({device, view});
	swapchain.extent = vkbSwapchain.extent;
	swapchain.format = vk::Format(vkbSwapchain.image_format);

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	vk::ImageCreateInfo dImageInfo = {
		.imageType = vk::ImageType::e2D,
		.format = vk::Format::eD16Unorm,
		.extent = {
			(unsigned)width,
			(unsigned)height,
			1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment
	};
	
	auto imageN = swapchain.images.size();

	swapchain.depthImages.clear();
	swapchain.depthImageViews.clear();
	swapchain.depthImages.reserve(imageN);
	swapchain.depthImageViews.reserve(imageN);
	for(int i = 0; i < imageN; ++i)
	{
		swapchain.depthImages.emplace_back(allocator, dImageInfo, vma::MemoryUsage::eGpuOnly, vk::MemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		vk::ImageViewCreateInfo dViewInfo = {
			.image = swapchain.depthImages.back().image,
			.viewType = vk::ImageViewType::e2D,
			.format = vk::Format::eD16Unorm,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eDepth,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},	
		};

		swapchain.depthImageViews.emplace_back(device, dViewInfo);
	}
	shouldDraw = true;
}

void Renderer::CreateRenderPass()
{
	vk::AttachmentDescription colorAttachment{
		.format = swapchain.format,
		.samples = vk::SampleCountFlagBits::e1, 
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR,
	};

	vk::AttachmentReference colorAttachmentRef{
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal,
	};

	vk::AttachmentDescription depthAttachment = {
		.format = vk::Format::eD16Unorm,
		.samples = vk::SampleCountFlagBits::e1, 
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eClear,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,	
	};

	vk::AttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
	};

	vk::SubpassDescription subpass{
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	vk::AttachmentDescription attachments[2] = {colorAttachment, depthAttachment};
	vk::RenderPassCreateInfo renderPassInfo{
		.attachmentCount = 2,
		.pAttachments = &attachments[0],
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};

	/* 
	vk::SubpassDependency colorDependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
		.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
		.srcAccessMask = vk::AccessFlagBits::eNoneKHR,		
		.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
	};
	vk::SubpassDependency depthDependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
		.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
		.srcAccessMask = vk::AccessFlagBits::eNoneKHR,		
		.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
	};
	
	vk::SubpassDependency dependencies[2] = {colorDependency, depthDependency};
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];
	*/

	
	renderPass = {device, renderPassInfo};
}	


void Renderer::CreateFramebuffers()
{
	swapchain.framebuffers.clear();
	if(shouldDraw)
	{
		swapchain.framebuffers.reserve(swapchain.imageViews.size());
		for (size_t i = 0; i < swapchain.imageViews.size(); i++) 
		{
			vk::ImageView attachments[2] = {*swapchain.imageViews[i], *swapchain.depthImageViews[i]};
			vk::FramebufferCreateInfo framebufferInfo{
				.renderPass = *renderPass,
				.attachmentCount = 2,
				.pAttachments = attachments,
				.width = swapchain.extent.width,
				.height = swapchain.extent.height,
				.layers = 1,
			};

			swapchain.framebuffers.push_back({device, framebufferInfo});
		}
	}
}

void Renderer::CreateCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo{
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Optional
		.queueFamilyIndex = indices.graphics,
	};

	commandPool = {device, poolInfo};

	vk::CommandBufferAllocateInfo allocInfo{
		.commandPool = *commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = bufferedFrames,
	};
	cmds = std::move(vk::raii::CommandBuffers(device, allocInfo));

	//Secondary commands pools
	for(int i = 0; i < externalDrawThreads; ++i)
	{
		secondaryCommandPools.emplace_back(device, poolInfo);
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = *secondaryCommandPools.back(),
			.level = vk::CommandBufferLevel::eSecondary,
			.commandBufferCount = bufferedFrames,
		};
		secondaryCmds[i] = std::move(vk::raii::CommandBuffers(device, allocInfo));
	}

	//Upload commands
	upload.cmdPool = {device, {.queueFamilyIndex = indices.graphics}};
	allocInfo = {
		.commandPool = *upload.cmdPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	upload.cmd = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
}

void Renderer::CreateDescriptorPools()
{
	std::vector<vk::DescriptorPoolSize> sizes =
	{
		{vk::DescriptorType::eUniformBuffer, 10},
		{vk::DescriptorType::eUniformBufferDynamic, 10 },
		{vk::DescriptorType::eStorageBuffer, 10 },
		{vk::DescriptorType::eCombinedImageSampler, 10 }
	};
	vk::DescriptorPoolCreateInfo pool_info = {
		.maxSets = 10,
		.poolSizeCount = (uint32_t)sizes.size(),
		.pPoolSizes = sizes.data(),
	};
	descriptorPool = {device, pool_info};
}

void Renderer::CreateSyncStructs()
{
	vk::SemaphoreCreateInfo semaphoreInfo;
	vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};
	for (auto &frame : frames) {
		frame.imageAvailableSemaphore = {device, semaphoreInfo};
		frame.renderFinishedSemaphore = {device, semaphoreInfo};
		frame.inFlightFence = {device, fenceInfo}; 
	}

	upload.fence = {device, vk::FenceCreateInfo{}}; 
}

bool Renderer::HandleEvents(SDL_Event event)
{
	switch(event.type)
	{
		case SDL_WINDOWEVENT:
			switch(event.window.event)
			{
				case SDL_WINDOWEVENT_MINIMIZED:
					shouldDraw = false;
					return true;
				case SDL_WINDOWEVENT_RESTORED:
					shouldDraw = true;
					return true;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					RecreateSwapchain();
					return true;
			}
			break;
	}
	return false;
}

void Renderer::RecreateSwapchain()
{
	device.waitIdle();
	CreateSwapchain();
	CreateFramebuffers();
}

bool Renderer::Acquire()
{
	const auto &inFlightFence = *frames[currentFrame].inFlightFence;
	const auto &imageAvailableSemaphore = *frames[currentFrame].imageAvailableSemaphore;

	if(!shouldDraw)
		return false;
	if(device.waitForFences(inFlightFence, VK_TRUE, UINT64_MAX) == vk::Result::eTimeout)
		return false;

	vk::Result imageCode;

	try{
		std::tie(imageCode, imageIndex) = swapchain.handle.acquireNextImage(UINT64_MAX, imageAvailableSemaphore);
		BeginDrawing(imageIndex);
		device.resetFences(inFlightFence);
	}
	catch(vk::OutOfDateKHRError e)
	{
		RecreateSwapchain();
		return false;
	}
	return true;	
}

void Renderer::Submit()
{
	if(!shouldDraw)
		return;
	EndDrawing(imageIndex);
	const auto &inFlightFence = *frames[currentFrame].inFlightFence;
	const auto &imageAvailableSemaphore = *frames[currentFrame].imageAvailableSemaphore;
	const auto &renderFinishedSemaphore = *frames[currentFrame].renderFinishedSemaphore;
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::SubmitInfo submitInfo{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &imageAvailableSemaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &*cmds[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderFinishedSemaphore
	};
	qGraphics.submit(submitInfo, inFlightFence);

	vk::PresentInfoKHR presentInfo{
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderFinishedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &*swapchain.handle,
		.pImageIndices = &imageIndex,
		.pResults = nullptr, // Optional
	};
	/* try
	{ */
		vk::Result presentCode = qPresent.presentKHR(presentInfo);
	/* }
	catch(vk::OutOfDateKHRError e)
	{
		RecreateSwapchain();
	} */
	currentFrame = (currentFrame + 1) % bufferedFrames;
}

void Renderer::SetClearColor(float color[4])
{
	memcpy(clearColor.data(), color, sizeof(float)*4);
}

void Renderer::BeginDrawing(int imageIndex)
{
	auto &cmd = cmds[currentFrame];
	cmd.reset();

	vk::CommandBufferBeginInfo beginInfo{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = nullptr,
	};

	//float add = (currentFrame+1)/5.f;
	const vk::ClearValue clearValues[2] = {
		vk::ClearColorValue{{clearColor}},
		{.depthStencil = vk::ClearDepthStencilValue{1.f, 0}}
	};

	vk::RenderPassBeginInfo renderPassInfo{
		.renderPass = *renderPass,
		.framebuffer = *swapchain.framebuffers[imageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = swapchain.extent,
		},
		.clearValueCount = 2,
		.pClearValues = clearValues,
	};
	
	//Secondary commands
	vk::CommandBufferInheritanceInfo inheritance{
		.renderPass = *renderPass,
		.subpass = 0,
		.framebuffer = *swapchain.framebuffers[imageIndex],
	};
	vk::CommandBufferBeginInfo beginSecondary{
		.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue | vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = &inheritance
	};
	for(int i = 0; i < externalDrawThreads; ++i)
	{
		auto &cmd2 = secondaryCmds[i][currentFrame];
		cmd2.reset();
		cmd2.begin(beginSecondary);
		cmd2.setViewport(0, viewportArea);
		cmd2.setScissor(0, scissorArea);
	}

	cmd.begin(beginInfo);
	cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);
}

void Renderer::EndDrawing(int imageIndex)
{
	std::array<vk::CommandBuffer, externalDrawThreads> executeCmds;
	auto &cmd = cmds[currentFrame];
	for(int i = 0; i < externalDrawThreads; ++i)
	{
		auto &cmd2 = secondaryCmds[i][currentFrame];
		cmd2.end();
		executeCmds[i] = *cmd2;
	}
	cmd.executeCommands(executeCmds);
	cmd.endRenderPass();
	cmd.end();
}

Renderer::Texture Renderer::LoadTextureSingle(const LoadTextureInfo& info)
{
	AllocatedBuffer ab;
	Texture texture = LoadAllocateTexture(info, ab);
	auto texPtr = &texture;
	auto f = std::bind(&Renderer::UploadTextures, std::placeholders::_1, &ab.buffer, &texPtr, 1);
	ExecuteCommand(f);
	return texture;
}

Renderer::Texture Renderer::LoadAllocateTexture(const LoadTextureInfo& info, AllocatedBuffer &stagingBuffer)
{
	const auto allocateFunc = [this, &stagingBuffer](size_t size)->void*{
		if(size > stagingBuffer.copySize)
			stagingBuffer.Allocate(this, size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
		return stagingBuffer.Map();
	};
	const auto deallocateFunc = [](void**){}; //They stay allocated. TODO: Method to free these staging buffers.
	void *dummyPtr; //Can't just pass null because ImageData uses it to write data to. It's what gets mapped above.
	
	ImageData image(ImageData::Allocator{&dummyPtr, allocateFunc, deallocateFunc});
	bool result;
	switch(info.type)
	{
		case palette:{
			auto size = info.height*info.width*4;
			stagingBuffer.Allocate(this, size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
			void * data = stagingBuffer.Map();
			image.width = info.width;
			image.height = info.height;
			image.bytesPerPixel = 4;
			memcpy(data, info.data, size);
			result = true;
			break;
		}
		default:
			result = image.LoadAny(info.path);
	}

	if(!result)
	{
		std::cerr << "Error while loading " << info.path <<"\n";
		throw std::runtime_error("Texture can't be loaded");
	}
	
	stagingBuffer.Unmap();
	vk::Extent3D extent = {image.width, image.height, 1};
	vk::Format format;
	if(image.compressed)
		format = vk::Format::eBc3SrgbBlock;
	else if(image.bytesPerPixel == 1)
		format = vk::Format::eR8Uint;
	else if(image.bytesPerPixel == 4)
		format = vk::Format::eR8G8B8A8Srgb;
	else
		assert(0 && "Unsupported format: ");


	vk::ImageCreateInfo imgInfo =
	{
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
	};
	
	AllocatedImage buf(allocator, imgInfo, vma::MemoryUsage::eGpuOnly);

	vk::ImageViewCreateInfo viewInfo = {
		.image = buf.image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	};

	return Texture{
		.buf = std::move(buf),
		.view = {device, viewInfo},
		.extent = extent,
		.format = format,
	};
}

void Renderer::UploadTextures(const vk::CommandBuffer &cmd, vk::Buffer *buffers, Texture **textures, size_t amount)
{
	std::vector<vk::ImageMemoryBarrier> barriers;
	barriers.reserve(amount);

	for(size_t i = 0; i < amount; ++i)
	{
		barriers.push_back(vk::ImageMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eNoneKHR,
			.dstAccessMask = vk::AccessFlagBits::eTransferWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eTransferDstOptimal,
			.image = textures[i]->buf.image,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		});
	}
	//barrier the image into the transfer-receive layout
	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
	barriers.clear();

	for(size_t i = 0; i < amount; ++i)
	{
		const auto &texture = *textures[i];
		vk::BufferImageCopy copyRegion = {
			.imageSubresource{
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageExtent = texture.extent
		};
		//copy the buffer into the image
		cmd.copyBufferToImage(buffers[i], texture.buf.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
		barriers.push_back(vk::ImageMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.oldLayout = vk::ImageLayout::eTransferDstOptimal,
			.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
			.image = texture.buf.image,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		});
	}

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
}

void Renderer::LoadTextures(const std::vector<LoadTextureInfo>& infos, std::vector<Texture> &textures)
{
	const auto amount = infos.size();
	std::vector<int> returns(amount);

	if(amount > stagingBuffers.size())
		stagingBuffers.resize(amount);
	textures.reserve(textures.size()+amount);

	std::vector<Texture*> textureRef(amount);
	std::vector<vk::Buffer> bufferRef(amount);

	for(int i = 0; i < amount; ++i)
	{
		textures.push_back(LoadAllocateTexture(infos[i], stagingBuffers[i]));
		textureRef[i] = &textures.back();
		bufferRef[i] = stagingBuffers[i].buffer;
	}

	auto f = std::bind(&Renderer::UploadTextures, std::placeholders::_1, bufferRef.data(), textureRef.data(), amount);
	ExecuteCommand(f);
}

void Renderer::ExecuteCommand(std::function<void(vk::CommandBuffer)>&& function)
{
	auto& cmd = *upload.cmd;
	cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	function(cmd);
	cmd.end();

	vk::SubmitInfo submitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd,
		};
	qGraphics.submit(submitInfo, *upload.fence);
	if(device.waitForFences(*upload.fence, VK_TRUE, -1) == vk::Result::eTimeout)
		throw std::runtime_error("Timeout on execute single command");
	device.resetFences(*upload.fence);
	upload.cmdPool.reset();
}

void Renderer::TransferBuffer(AllocatedBuffer &src, AllocatedBuffer &dst, size_t size)
{
	ExecuteCommand([&](vk::CommandBuffer cmd) {
		vk::BufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;
		cmd.copyBuffer(src.buffer, dst.buffer, 1, &copy);
	});
}

PipelineBuilder Renderer::GetPipelineBuilder()
{
	return {&device, this};
}

vk::raii::Pipeline Renderer::RegisterPipelines(const vk::GraphicsPipelineCreateInfo& pipelineInfo)
{
	return {device, nullptr, pipelineInfo};
}

std::pair<vk::raii::Pipeline, vk::raii::PipelineLayout> Renderer::RegisterPipelines(vk::GraphicsPipelineCreateInfo& pipelineInfo, const vk::PipelineLayoutCreateInfo& pipelineLayoutInfo)
{
	vk::raii::PipelineLayout layout = {device, pipelineLayoutInfo};
	pipelineInfo.layout = *layout;
	pipelineInfo.renderPass = *renderPass;
	//pipelineInfo.basePipelineHandle = lastPipeline;
	vk::raii::Pipeline graphicsPipeline = {device, nullptr, pipelineInfo};
	//lastPipeline = *graphicsPipeline;

	return {std::move(graphicsPipeline), std::move(layout)};
}

std::vector<vk::DescriptorSet> Renderer::CreateDescriptorSets(const std::vector<vk::DescriptorSetLayout>& layouts)
{
	vk::DescriptorSetAllocateInfo setAllocInfo ={
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = (uint32_t)layouts.size(),
		.pSetLayouts = layouts.data(),
	};
	return (*device).allocateDescriptorSets(setAllocInfo);
}

const vk::CommandBuffer *Renderer::GetCommand()
{
	if(!shouldDraw)
		return nullptr;
	else
		return &*secondaryCmds[0][currentFrame];
}

void Renderer::Wait()
{
	device.waitIdle();
}

vk::raii::Sampler Renderer::CreateSampler(vk::Filter filter, vk::SamplerAddressMode mode)
{
	vk::SamplerCreateInfo samplerI{
		.magFilter = filter,
		.minFilter = filter,
		.addressModeU = mode,
		.addressModeV = mode,
		.addressModeW = mode,
	};
	return {device, samplerI};
}

// Calculate required alignment based on minimum device offset alignment
size_t Renderer::PadToAlignment(size_t originalSize)
{
	size_t alignedSize = originalSize;
	if (uniformBufferAlignment > 0) {
		alignedSize = (alignedSize + uniformBufferAlignment - 1) & ~(uniformBufferAlignment - 1);
	}
	return alignedSize;
}

VkRenderPass Renderer::GetRenderPass()
{
	return *renderPass;
}

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	std::cerr << "[imgui] vulkan error: VkResult = "<<err<<"\n";
	if (err < 0)
		throw std::runtime_error("Imgui-Vulkan error");
}



Renderer::ImguiInfo Renderer::GetImguiInfo()
{
	return ImguiInfo{
		.Instance = *instance,
		.PhysicalDevice = *pDevice,
		.Device = *device,
		.QueueFamily = indices.graphics,
		.Queue = qGraphics,
		.PipelineCache = VK_NULL_HANDLE,
		.DescriptorPool = *descriptorPool,
		.Subpass = 0,
		.MinImageCount = bufferedFrames,
		.ImageCount = (uint32_t)swapchain.images.size(),
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.Allocator = nullptr,
		.CheckVkResultFn = check_vk_result,
	};
}