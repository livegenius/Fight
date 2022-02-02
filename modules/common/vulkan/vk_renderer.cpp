#include "vk_renderer.h"
#include <VkBootstrap.h>
#include <SDL_vulkan.h>
#include <iostream>

size_t VulkanRenderer::uniformBufferAligment = 0;

bool VulkanRenderer::Init(SDL_Window *window, int syncMode)
{
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

	return true;
}

void VulkanRenderer::Submit()
{

}

bool VulkanRenderer::HandleEvents(SDL_Event)
{
	return false;
}