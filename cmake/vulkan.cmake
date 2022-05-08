find_package(Vulkan REQUIRED COMPONENTS glslc)

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/data/spirv)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
function(compile_shader target)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "FORMAT" "SOURCES")
	foreach(source ${arg_SOURCES})
		get_filename_component(filename ${source} NAME)
		set(outFilePath "${PROJECT_SOURCE_DIR}/data/spirv/${filename}.${arg_FORMAT}")
		add_custom_command(
			OUTPUT ${outFilePath}
			DEPENDS ${source}
			DEPFILE ${filename}.d
			COMMAND
				${glslc_executable}
				#$<$<BOOL:${arg_ENV}>:--target-env=${arg_ENV}>
				--target-env=vulkan1.2
				$<$<BOOL:${arg_FORMAT}>:-mfmt=${arg_FORMAT}>
				-MD -MF ${filename}.d
				-o ${outFilePath}
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
		)
		target_sources(${target} PRIVATE ${outFilePath})
	endforeach()
endfunction()

add_library(vulkan-pch INTERFACE)
target_compile_definitions(vulkan-pch INTERFACE VULKAN_HPP_NO_CONSTRUCTORS)

if(false)
	target_precompile_headers(vulkan-pch INTERFACE
		<vulkan/vulkan.hpp>
		<vulkan/vulkan_raii.hpp>
	)
endif()

add_subdirectory(submodules/vk-bootstrap)

#Add VMA library
add_library(vma-hpp STATIC
	submodules/vma-hpp/vmahppimp.cpp 
)
target_include_directories(vma-hpp PUBLIC submodules/vma-hpp)
target_link_libraries(vma-hpp PRIVATE Vulkan::Vulkan vulkan-pch)

if(false)
	target_precompile_headers(vma-hpp PUBLIC
		#vk_mem_alloc.h doesn't play well with precompiled headers
		[["vk_mem_alloc.hpp"]]
	)
endif()

#Add SPIRV-Reflect
add_library(spirv-reflect STATIC
	submodules/spirv-reflect/spirv_reflect.c
)
target_include_directories(spirv-reflect PUBLIC submodules/spirv-reflect)
target_link_libraries(spirv-reflect PRIVATE Vulkan::Vulkan)


