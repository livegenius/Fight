find_package(Vulkan REQUIRED COMPONENTS glslc)

message(STATUS "Vulkan supported.")

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/data/spirv)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
function(compile_shader target)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "FORMAT" "SOURCES")
	foreach(source ${arg_SOURCES})
		get_filename_component(filename ${source} NAME)
		add_custom_command(
			OUTPUT ${source}.${arg_FORMAT}
			DEPENDS ${source}
			DEPFILE ${source}.d
			COMMAND
				${glslc_executable}
				#$<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
				--target-env=vulkan1.2
				$<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
				-MD -MF ${source}.d
				-o ${PROJECT_SOURCE_DIR}/data/spirv/${filename}.${arg_FORMAT}
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
		)
		target_sources(${target} PRIVATE ${source}.${arg_FORMAT})
	endforeach()
endfunction()

add_subdirectory(submodules/vk-bootstrap)

#Add VMA library
add_library(vma-hpp STATIC
	submodules/vma-hpp/vmahppimp.cpp 
)
target_include_directories(vma-hpp PUBLIC submodules/vma-hpp)
target_link_libraries(vma-hpp PRIVATE Vulkan::Vulkan)
target_precompile_headers(vma-hpp PUBLIC
	#vk_mem_alloc.h doesn't play well with precompiled headers
	[["vk_mem_alloc.hpp"]]
	<vulkan/vulkan.hpp>
	<vulkan/vulkan_raii.hpp>
)

target_compile_definitions(Vulkan::Vulkan INTERFACE VULKAN_HPP_NO_CONSTRUCTORS)

