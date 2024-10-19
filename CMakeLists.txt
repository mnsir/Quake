project(Quake C CXX)

# "-startwindowed",
#    //"-playback",
#    //"-startwindowed -record",
#    "-cachedir -width 800 -height 600 -bpp 32 -window -no8bit"

find_package(OpenGL REQUIRED)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(QuakeGL)
add_subdirectory(QuakeHW)
add_subdirectory(common)
