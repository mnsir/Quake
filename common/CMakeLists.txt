add_library(common STATIC
	"time.h"
	"time.cpp"
	"pak.h"
	"pak.cpp"
)

add_library(quake::common ALIAS common)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
