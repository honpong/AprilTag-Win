cmake_minimum_required(VERSION 3.7.0)

project(shapefit C CXX)

add_subdirectory(src)

if(MSVC)
    if(BUILD_SHAPEFIT_APP)
        set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT shapefit_app)
    endif()
endif()
