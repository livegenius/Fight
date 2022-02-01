add_library(Graphics STATIC)

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
	target_compile_definitions(Graphics PUBLIC VULKAN_RENDERER)
	target_link_libraries(Graphics PUBLIC Vulkan::Vulkan vk-bootstrap::vk-bootstrap vma-hpp)
endif()
if(WITH_OPENGL)
	target_compile_definitions(Graphics PUBLIC OPENGL_RENDERER)
	target_link_libraries(Graphics PUBLIC glad::glad)
endif()

target_include_directories(Graphics PUBLIC ".")
target_sources(Graphics PRIVATE
	#shader.cpp
	#texture.cpp
	#vao.cpp
	#ubo.cpp
	#gfx_handler.cpp
	particle.cpp
	xorshift.cpp
)

target_link_libraries(Graphics PUBLIC Image lz4_static)
target_link_libraries(Graphics PUBLIC sol2::sol2)
