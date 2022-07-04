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
	
	gfx_handler.cpp
	hitbox_renderer.cpp
	
	particle.cpp
	xorshift.cpp

	framedata_io.cpp
)

target_link_libraries(Common PUBLIC Image SDL2::SDL2 glm::glm sol2::sol2)
target_link_libraries(Common PRIVATE header_only lz4_static)



compile_shader(Common
	FORMAT bin
	SOURCES
		shader/sprite.vert
		shader/sprite.frag
		shader/particle.vert
		shader/particle.frag
		shader/box.vert
		shader/box.frag
		shader/hud.vert
		shader/hud.frag
) 