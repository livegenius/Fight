#include "renderer.h"

#if defined (VULKAN_RENDERER)
	#include "vulkan/vk_renderer.h"
#elif defined (OPENGL_RENDERER)
	#include "opengl/gl_renderer.h"
#endif

Renderer* Renderer::NewRenderer()
{
	#if defined (VULKAN_RENDERER)
		return new VulkanRenderer;
	#elif defined (OPENGL_RENDERER)
		return new OpenglRenderer;
	#endif
}