#include "vk_renderer.h"
#include <VkBootstrap.h>
#include <SDL_vulkan.h>
#include <iostream>
#include <image.h>

size_t VulkanRenderer::uniformBufferAligment = 0;

bool VulkanRenderer::Init(SDL_Window *window_, int syncMode)
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
			.require_present()
			.set_minimum_version(1, 2)
			.add_desired_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
			.select ();
	if (!phys_device_ret) {
		std::cerr <<("Couldn't select physical device.\n");
		return false;
	}
	auto phys_dev = phys_device_ret.value();
	pDevice = {instance, phys_dev.physical_device};

	deviceProperties = phys_dev.properties;
	uniformBufferAligment = deviceProperties.limits.minUniformBufferOffsetAlignment;

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

	CreateSwapchain();
	CreateRenderPass();
	CreateFramebuffers();
	CreateCommandPool();
	CreateSyncStructs();

	return true;
}

void VulkanRenderer::CreateSwapchain()
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
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
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

void VulkanRenderer::CreateRenderPass()
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


void VulkanRenderer::CreateFramebuffers()
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

void VulkanRenderer::CreateCommandPool()
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

	//Upload commands
	upload.cmdPool = {device, {.queueFamilyIndex = indices.graphics}};
	allocInfo = {
		.commandPool = *upload.cmdPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	upload.cmd = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
}

void VulkanRenderer::CreateSyncStructs()
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

bool VulkanRenderer::HandleEvents(SDL_Event event)
{
	switch(event.type)
	{
		case SDL_WINDOWEVENT:
			switch(event.window.event)
			{
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					RecreateSwapchain();
					return true;
			}
			break;
	}
	return false;
}

void VulkanRenderer::RecreateSwapchain()
{
	device.waitIdle();
	CreateSwapchain();
	CreateFramebuffers();
}

void VulkanRenderer::Submit()
{
	const auto &inFlightFence = *frames[currentFrame].inFlightFence;
	const auto &imageAvailableSemaphore = *frames[currentFrame].imageAvailableSemaphore;
	const auto &renderFinishedSemaphore = *frames[currentFrame].renderFinishedSemaphore;

	if(!shouldDraw)
		return;
	if(device.waitForFences(inFlightFence, VK_TRUE, UINT64_MAX) == vk::Result::eTimeout)
		return;

	uint32_t imageIndex;
	vk::Result imageCode;

	try{
		std::tie(imageCode, imageIndex) = swapchain.handle.acquireNextImage(UINT64_MAX, imageAvailableSemaphore);

		//Draw(imageIndex);
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

		Draw(imageIndex);
		device.resetFences(inFlightFence);
		
		qGraphics.submit(submitInfo, inFlightFence);

		vk::PresentInfoKHR presentInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &*swapchain.handle,
			.pImageIndices = &imageIndex,
			.pResults = nullptr, // Optional
		};
		vk::Result presentCode = qPresent.presentKHR(presentInfo);
	}
	catch(vk::OutOfDateKHRError e)
	{
		RecreateSwapchain();
	}

	//vkQueueWaitIdle(lDevice.qPresent);
	currentFrame = (currentFrame + 1) % bufferedFrames;
}

void VulkanRenderer::Draw(int imageIndex)
{
	auto &cmd = cmds[currentFrame];
	cmd.reset();

	vk::CommandBufferBeginInfo beginInfo{
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
		.pInheritanceInfo = nullptr,
	};

	const vk::ClearValue clearValues[2] = {
		vk::ClearColorValue{{{ 0.2f, 0.0f, 0.2f, 1.0f }}},
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

	cmd.begin(beginInfo);
	cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	cmd.endRenderPass();
	cmd.end();
}

std::vector<int> VulkanRenderer::LoadTextures(std::vector<LoadTextureInfo>& infos)
{
	std::vector<int> returns(infos.size());
	std::vector<AllocatedBuffer> stagingBuffers(infos.size());
	textures.reserve(textures.size()+infos.size());

	struct TextureWRef{
		size_t texture;
		int stagingIndex;
	};
	std::vector<TextureWRef> newTextures;

	//std::vector<AllocatedBuffer>
	for(int i = 0; i < infos.size(); ++i)
	{
		auto &info = infos[i];
		AllocatedBuffer &stagingBuffer = stagingBuffers[i];
		auto allocateFunc = [this, &stagingBuffer](size_t size)->void*{
			stagingBuffer.Allocate(allocator, size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);
			return stagingBuffer.Map();
		};

		char *ptr;
		auto deallocateFunc = [](void**){};
		ImageData image;
		bool result;
		if(info.type == TextureType::lzs3)
			result = image.LoadLzs3(info.path, ImageData::Allocation{(void**)&ptr, allocateFunc, deallocateFunc});
		else if(info.type == TextureType::png)
			result = image.LoadPng(info.path, ImageData::Allocation{(void**)&ptr, allocateFunc, deallocateFunc});
		
		stagingBuffer.Unmap();
		if(!result)
		{
			std::cerr << "Error while loading " << info.path <<"\n";
			throw std::runtime_error("Texture can't be loaded");
		}

		vk::Extent3D extent = {image.width, image.height, 1};
		vk::Format format;
		if(image.bytesPerPixel == 1)
			format = vk::Format::eR8Uint;
		else if(image.bytesPerPixel == 4)
			format = vk::Format::eR8G8B8A8Srgb;
		else
			throw std::runtime_error(std::string("Unsupported format: ") + info.path.string());

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

		textures.emplace(textureMapCounter, Texture{
			.buf = std::move(buf),
			.view = {device, viewInfo},
			.extent = extent,
			.format = format,
		});

		newTextures.push_back({textureMapCounter, i});

		returns[i] = textureMapCounter;
		textureMapCounter++;
	}

	ExecuteCommand([&](vk::CommandBuffer cmd) {

		std::vector<vk::ImageMemoryBarrier> barriers;
		
		for(const auto &ref : newTextures)
		{
			const auto &texture = textures.at(ref.texture);
			barriers.push_back(vk::ImageMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eNoneKHR,
				.dstAccessMask = vk::AccessFlagBits::eTransferWrite,
				.oldLayout = vk::ImageLayout::eUndefined,
				.newLayout = vk::ImageLayout::eTransferDstOptimal,
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
		//barrier the image into the transfer-receive layout
		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
		barriers.clear();

		for(const auto &ref : newTextures)
		{
			const auto &texture = textures.at(ref.texture);
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
			cmd.copyBufferToImage(stagingBuffers[ref.stagingIndex].buffer, texture.buf.image, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
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
	});
	
	return returns;
}

void VulkanRenderer::ExecuteCommand(std::function<void(vk::CommandBuffer)>&& function)
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

int VulkanRenderer::NewBuffer(size_t size, BufferFlags interfaceFlags, int oldBuffer)
{
	vk::BufferUsageFlags flags;
	vma::MemoryUsage vmaFlags;

	switch(interfaceFlags)
	{
		case VertexStatic:
			flags = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
			vmaFlags = vma::MemoryUsage::eGpuOnly;
			break;
		case TransferSrc:
			flags = vk::BufferUsageFlagBits::eTransferSrc;
			vmaFlags = vma::MemoryUsage::eCpuOnly;
			assert(oldBuffer != -1);
			break;
	}
	buffers.emplace(bufferMapCounter, Buffer{{allocator, size, flags, vmaFlags}, interfaceFlags});
	bufferMapCounter++;
	return bufferMapCounter-1;
}

void VulkanRenderer::DestroyBuffer(int handle)
{
	buffers.erase(handle);
}

void* VulkanRenderer::MapBuffer(int handle)
{
	return buffers[handle].buf.Map();
}

void VulkanRenderer::UnmapBuffer(int handle)
{
	buffers[handle].buf.Unmap();
}

void VulkanRenderer::TransferBuffer(int src, int dst, size_t size)
{
	ExecuteCommand([&](vk::CommandBuffer cmd) {
		vk::BufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = size;
		cmd.copyBuffer(buffers[src].buf.buffer, buffers[dst].buf.buffer, 1, &copy);
	});
}