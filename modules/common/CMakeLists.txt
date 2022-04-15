add_library(Common STATIC)

target_include_directories(Common PUBLIC vk)
target_link_libraries(Common PUBLIC vulkan-pch Vulkan::Vulkan vk-bootstrap::vk-bootstrap vma-hpp spirv-reflect)
target_sources(Common PRIVATE
	vk/renderer.cpp
	vk/allocation.cpp
	vk/pipeline_builder.cpp
	vk/format_info.cpp
)

target_include_directories(Common PUBLIC ".")
target_sources(Common PRIVATE
	vertex_buffer.cpp

	shader.cpp
	texture.cpp
	vao.cpp
	ubo.cpp
	gfx_handler.cpp
	
	particle.cpp
	xorshift.cpp
)

target_link_libraries(Common PUBLIC Image SDL2::SDL2 glm::glm)
target_link_libraries(Common PUBLIC sol2::sol2)



compile_shader(Common
	FORMAT bin
	SOURCES
		shader/sprite.vert
		shader/sprite.frag
		shader/particle.vert
		shader/particle.frag
) 