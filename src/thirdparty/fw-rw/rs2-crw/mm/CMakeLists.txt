
# Set minimum required CMake version
cmake_minimum_required(VERSION 3.1.0)

# Name the project
project (rs2-crw-mm)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "/MT")

# Source files
FILE(GLOB CUSTOM_MM_RW_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Add the sources to the target

if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${CUSTOM_MM_RW_SRC})
else()
    add_library(${PROJECT_NAME} STATIC ${CUSTOM_MM_RW_SRC})
endif()


target_include_directories(${PROJECT_NAME} PUBLIC inc ${LIBRS_DIR}/include)

set(DEPENDENCIES realsense2)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})

