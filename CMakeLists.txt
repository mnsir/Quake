cmake_minimum_required(VERSION 3.26.0 FATAL_ERROR)

project(Quake C CXX)

find_package(OpenGL REQUIRED)

add_subdirectory(QuakeGL)
add_subdirectory(QuakeHW)
