add_executable(Fight)

target_include_directories(Fight PRIVATE ".")
target_sources(Fight PRIVATE
	resource.rc
	audio.cpp
	camera.cpp
	chara.cpp
	command_inputs.cpp
	hud.cpp
	main.cpp
	netplay.cpp
	raw_input.cpp
	util.cpp
	window.cpp
	battle_scene.cpp
	framedata.cpp
	actor.cpp
	stage.cpp
)

target_link_libraries(Fight PRIVATE
	#Internal
	vulkan-pch
	FixedPoint
	Geometry
	Image
	Common

	#External
	sol2::sol2
	enet
	ggpo
)

#External
target_link_libraries(Fight PRIVATE
	glm::glm
	SDL2::SDL2 SDL2::SDL2main
	soloud
)

if(WIN32)
	target_link_libraries(Fight PRIVATE winmm)
	target_compile_definitions(Fight PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()


#install(TARGETS Fight RUNTIME DESTINATION debug)