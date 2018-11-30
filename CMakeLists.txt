cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

project(measure C CXX)

list(APPEND CMAKE_MODULE_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)
list(APPEND CMAKE_PREFIX_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/../sp/win
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/sp/win
)

string(TIMESTAMP time_str "%Y%m%d%H%M")
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/boxsdk2_${time_str})
message(STATUS "boxsdk_install ${CMAKE_INSTALL_PREFIX}")

######################################################################

set(LIBRS_GIT https://github.com/IntelRealSense/librealsense.git)
set(LIBRS_VER v2.16.2)
set(LIBRS_PAR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/librealsense)
set(LIBRS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/librealsense/librealsense)
set(JSON3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/Json)

if(NOT EXISTS ${LIBRS_DIR})
    message(STATUS "cloning librealsense into ${LIBRS_PAR} ...")
    find_package(Git)
    if(Git_FOUND)
        message(STATUS "${GIT_EXECUTABLE} clone -—progress -v -b ${LIBRS_VER} ${LIBRS_GIT} ${LIBRS_DIR}")
        execute_process(COMMAND ${GIT_EXECUTABLE} clone --progress -v -b ${LIBRS_VER} ${LIBRS_GIT} ${LIBRS_DIR}
                        WORKING_DIRECTORY ${LIBRS_PAR})
    endif()
    
    if(NOT EXISTS ${LIBRS_DIR})
        message(FATAL_ERROR "Cannot clone librealsense! Please checkout ${LIBRS_GIT} into ${LIBRS_PAR} and try again.")
    endif()

    if(WIN32) #for error C3861: "__cpuid": identifier not found.
        file(APPEND ${LIBRS_DIR}/src/image.h
            "\n#ifndef LIBREALSENSE_INTRIN_H\n#define LIBREALSENSE_INTRIN_H\n#ifdef WIN32\n#include <intrin.h>\n#endif\n#endif\n")
    endif()
endif()

######################################################################
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
if(COMPILER_SUPPORTS_CXX14)
   #set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
else()
    message(FATAL_ERROR "Compiler does not support C++14!")
endif()

######################################################################

find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)

if(MSVC)

    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)

    # Makes VS15 find the DLL when trying to run examples
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_CONFIGURATION_TYPES Release Debug CACHE STRING "" FORCE)

    add_definitions(-D_USE_MATH_DEFINES) # we expect M_PI in math.h / cmath
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/sdl)
    find_package(glfw3 QUIET)
    if (NOT TARGET glfw)
      add_subdirectory(${LIBRS_DIR}/third-party/glfw)
      add_library(glfw ALIAS glfw3)
    end()

elseif(APPLE)

    set(BUILD_WITH_OPENMP OFF CACHE BOOL "" FORCE)
    set(FORCE_LIBUVC ON)

    find_package(PkgConfig)
    if(NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "\n\n PkgConfig package is missing!\n\n")
    endif()

    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
        include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
        link_directories(${LIBUSB1_LIBRARY_DIRS})
    else()
        message(FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    find_package(glfw3 REQUIRED)
endif()

#########################################################################
set(BUILD_EXAMPLES            OFF CACHE BOOL "")
set(BUILD_GRAPHICAL_EXAMPLES  OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "")
set(BUILD_UNIT_TESTS          OFF CACHE BOOL "" FORCE)
set(ALGO_LIB algo-core CACHE STRING "")

list(APPEND LIBRS_APP_LIBS json3 ${OPENGL_LIBRARIES})
list(APPEND LIBRS_LIBS realsense2)
list(APPEND LIBRS_LIBS glfw)

add_subdirectory(${JSON3_DIR})
add_subdirectory(${LIBRS_DIR})
add_subdirectory(src/api)
add_subdirectory(boxsdk)

if(MSVC)
    target_compile_options(realsense2 BEFORE PUBLIC /wd4996 /wd4703) # disable librealsense warnings
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT box-app)
endif()
