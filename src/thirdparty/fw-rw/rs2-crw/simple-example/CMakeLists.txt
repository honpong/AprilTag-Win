
# Set minimum required CMake version
cmake_minimum_required(VERSION 3.1.0)

# Name the project
project (simple_imu_rw)

# Find librealsense2 installed package
#find_package(realsense2 REQUIRED)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "/MT")

if(MSVC)
    add_definitions(-D_WIN32)
endif()

# Source files
    set(RS_SIMPLE_IMU_RW_CPP
            src/main.cpp
        )

# Add the sources to the target
add_executable(simple_imu_rw ${RS_SIMPLE_IMU_RW_CPP})

#include_directories(simple_imu_rw ../mm/inc ../librealsense/include)

set(DEPENDENCIES rs2-crw-mm realsense2)

target_link_libraries(simple_imu_rw  ${DEPENDENCIES})

