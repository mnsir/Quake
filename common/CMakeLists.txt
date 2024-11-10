add_library(common STATIC
	"pak.cpp"
	"pak.h"
	"progs.h"
	"progs_fielddefs.cpp"
	"progs_functions.cpp"
	"progs_globaldefs.cpp"
	"progs_globals.cpp"
	"progs_globals2.cpp"
	"progs_globals3.cpp"
	"progs_globals4.cpp"
	"progs_globals5.cpp"
	"progs_statements.cpp"
	"progs_strings.cpp"
	"time.cpp"
	"time.h"
)

target_precompile_headers(common PRIVATE pch.h)

add_library(quake::common ALIAS common)
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
