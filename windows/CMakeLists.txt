cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

add_library(RCTracker SHARED ../corvis/src/filter/rc_intel_interface.cpp)
target_link_libraries(RCTracker PRIVATE
  filter
  numerics
  cor
  vis
)
set_property(TARGET RCTracker PROPERTY CXX_STANDARD 14)

