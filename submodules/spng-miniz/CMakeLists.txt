add_library(spng OBJECT)
target_sources(spng PRIVATE
	"spng/spng.c"
	"miniz/miniz.c"
)

target_include_directories(spng PUBLIC spng)
target_include_directories(spng PRIVATE miniz)
target_compile_definitions(spng PUBLIC SPNG_STATIC SPNG_USE_MINIZ)
set_target_properties(spng PROPERTIES POSITION_INDEPENDENT_CODE ON)
