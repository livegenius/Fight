add_library(Common STATIC)

if(AFGE_RENDERER STREQUAL "any")
	if(Vulkan_FOUND)
		set(WITH_VULKAN True)
	else() #Maybe both in the future?
		set(WITH_OPENGL True)
	endif()
elseif(AFGE_RENDERER STREQUAL "vulkan")
	set(WITH_VULKAN True)
else()
	set(WITH_OPENGL True)
endif()

if(WITH_VULKAN)
	target_include_directories(Common PUBLIC vulkan)
	target_compile_definitions(Common PUBLIC VULKAN_RENDERER)
	target_link_libraries(Common PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap vma-hpp)
	target_sources(Common PRIVATE
		vulkan/vk_renderer.cpp
	)
endif()
if(WITH_OPENGL)
	#target_include_directories(Common PUBLIC opengl)
	target_compile_definitions(Common PUBLIC OPENGL_RENDERER)
	target_link_libraries(Common PUBLIC glad::glad)
endif()

target_include_directories(Common PUBLIC ".")
target_sources(Common PRIVATE
	renderer.cpp

	shader.cpp
	texture.cpp
	vao.cpp
	ubo.cpp
	gfx_handler.cpp
	
	particle.cpp
	xorshift.cpp
)

target_link_libraries(Common PUBLIC Image lz4_static SDL2::SDL2)
target_link_libraries(Common PUBLIC sol2::sol2)
