#Visual framedata editor
add_executable(Frametool)

target_link_libraries(Frametool PRIVATE
	Vulkan::Vulkan
	vulkan-pch
	glm::glm
	SDL2::SDL2
	SDL2::SDL2main
	imgui::imgui
	nfd

	Common
)

target_sources(Frametool PRIVATE
	main.cpp
	main_frame.cpp
	render.cpp
	window.cpp
	ini.cpp
	main_pane.cpp
	imgui_utils.cpp
)