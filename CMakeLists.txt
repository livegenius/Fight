cmake_minimum_required(VERSION 3.13...3.19)
project (AFGEngine VERSION 0.3 LANGUAGES C CXX)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "" )

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

set( AFGE_RENDERER "vulkan" CACHE STRING "Use renderer: vulkan, opengl, any")
set( AFGE_BUILD_TOOLS ON CACHE BOOL "Build tools to edit game data")
set( AFGE_USE_SUBMODULES ON CACHE BOOL "Use git submodules. Turn off if you want to use a package manager instead.")

if(NOT AFGE_RENDERER STREQUAL "opengl")
	include(vulkan)
endif()

if(NOT AFGE_RENDERER STREQUAL "vulkan")
	include(glad)
endif()

include(sol2)
include(includes)
add_subdirectory(submodules/spng-miniz)

if(AFGE_USE_SUBMODULES)
	add_subdirectory(submodules/glm)
	add_subdirectory(submodules/SDL)
	add_subdirectory(submodules/enet)
	add_subdirectory(submodules/ggpo)
	add_subdirectory(submodules/soloud/contrib)

	set(LZ4_BUILD_CLI OFF)
	set(LZ4_BUILD_LEGACY_LZ4C OFF)
	add_subdirectory(submodules/lz4/build/cmake)
	target_include_directories(lz4_static INTERFACE submodules/lz4/lib)
endif()

if(NOT AFGE_USE_SUBMODULES)
	find_package(PNG REQUIRED)
	find_package(SDL2 CONFIG REQUIRED)
	find_package(glm CONFIG REQUIRED)
endif()

add_subdirectory(modules)
add_subdirectory(engine)

if(AFGE_BUILD_TOOLS)
	add_subdirectory(submodules/squish) 
	#Not supported by non-submodule build
	include(imgui) #We use a special branch branch.
	add_subdirectory(submodules/nfd) 

	add_subdirectory(tools)
endif()

