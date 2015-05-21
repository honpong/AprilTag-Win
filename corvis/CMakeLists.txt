cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

set(PACKAGE_NAME "corvis")

project(${PACKAGE_NAME} C CXX)

add_subdirectory(src)

add_executable(measure
  src/tools/measure.cpp
  src/filter/replay.cpp
)
target_link_libraries(measure
  filter
  numerics
  cor
  vis
  mgl
)

