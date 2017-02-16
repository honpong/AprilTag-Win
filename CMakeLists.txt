cmake_minimum_required(VERSION 3.7.0)

project(shapefit C CXX)

add_subdirectory(src)

if(MSVC)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT shapefit_api)
endif()
