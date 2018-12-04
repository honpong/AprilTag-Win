
# Set minimum required CMake version
cmake_minimum_required(VERSION 3.1.0)

# Name the project custom calibration r/w
project (rs2-crw)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set( CMAKE_CXX_FLAGS " -pthread " )

if (MSVC)
set( CMAKE_CXX_FLAGS " /EHsc " )
endif()

add_subdirectory(mm)
add_subdirectory(simple-example)

if (NOT TARGET realsense2)

if(MSVC)

    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
#        if(${flag_var} MATCHES "/MD")
#            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#        endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)

    # Makes VS15 find the DLL when trying to run examples
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_CONFIGURATION_TYPES Release Debug CACHE STRING "" FORCE)

    add_definitions(-D_USE_MATH_DEFINES) # we expect M_PI in math.h / cmath
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32)
    add_definitions(-D_WIN32)
    add_compile_options(/sdl)
endif()

set(LIBRS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/librealsense)

set(BUILD_EXAMPLES            OFF CACHE BOOL "")
set(BUILD_GRAPHICAL_EXAMPLES  OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "")
set(BUILD_UNIT_TESTS          OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_TM2            OFF CACHE BOOL "" FORCE)
set(FORCE_WINUSB_UVC          OFF CACHE BOOL "" FORCE)
set(FORCE_LIBUVC              OFF CACHE BOOL "" FORCE)

add_subdirectory(${LIBRS_DIR})

if(MSVC)
    target_compile_options(realsense2 BEFORE PUBLIC /wd4996 /wd4703) # disable librealsense warnings
endif()

endif()
