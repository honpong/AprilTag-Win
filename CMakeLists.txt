cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

project(measure C CXX)

######################################################################

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

#######################################################################

set(LIBRS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdparty/LibRealSense/librealsense)

find_package(OpenGL REQUIRED)
set(LIBRS_LIBS realsense2 ${OPENGL_LIBRARIES})

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
    #add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_subdirectory(${LIBRS_DIR}/third-party/glfw)
    list(APPEND LIBRS_LIBS glfw3)

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

    # Find glfw header
    find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h PATHS
        /usr/X11R6/include
        /usr/include/X11
        /opt/graphics/OpenGL/include
        /opt/graphics/OpenGL/contrib/libglfw
        /usr/local/include
        /usr/include/GL
        /usr/include)

    # Find glfw library
    find_library(GLFW_LIBRARIES NAMES glfw glfw3 PATHS
        /usr/lib64
        /usr/lib
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
        /usr/local/lib64
        /usr/local/lib
        /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
        /usr/X11R6/lib)

    list(APPEND LIBRS_LIBS m ${LIBUSB1_LIBRARIES} ${GLFW_LIBRARIES})
    include_directories(${GLFW_INCLUDE_DIR})
endif()

#########################################################################

set(BUILD_EXAMPLES            OFF CACHE BOOL "" FORCE)
set(BUILD_GRAPHICAL_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "")
set(BUILD_UNIT_TESTS          OFF CACHE BOOL "" FORCE)
set(ALGO_LIB algo-core CACHE STRING "")
add_subdirectory(${LIBRS_DIR})
add_subdirectory(src/api)
add_subdirectory(boxsdk)

if(MSVC)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT box-app)
endif()