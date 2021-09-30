add_library(glad
	"${PROJECT_SOURCE_DIR}/submodules/glad/src/glad.c"
)

target_include_directories(glad PUBLIC
	"${PROJECT_SOURCE_DIR}/submodules/glad/include"
)

target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

add_library(glad::glad ALIAS glad)
