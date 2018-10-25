cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

if(ALGO_LIB)
    set(PROJECT_NAME ${ALGO_LIB})
    option(BUILD_SHARED_LIB_ALGO "Build Shared Algorithm Library" OFF)
else()
    set(PROJECT_NAME shapefit-core)
    option(BUILD_SHARED_LIB_SHAPEFIT "Build Shared Shapefit Library" ON)
    if(BUILD_SHARED_LIB_SHAPEFIT)
        set(BUILD_SHARED_LIB_ALGO ON CACHE STRING "")
    endif()
endif()

project(${PROJECT_NAME})

#############################################################################################
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../../cmake)
find_package(Eigen3 REQUIRED)

SET(ALGO_CORE_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc CACHE STRING "")
SET(OpenCV_STATIC ON)
find_package(OPENCV QUIET)
if(OPENCV_FOUND OR OpenCV_FOUND)
    add_definitions(-DOPENCV_FOUND)
endif()

#############################################################################################
FILE(GLOB ALGO_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB ALGO_CORE_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*" "${ALGO_CORE_HDR_DIR}/*")
if(BUILD_SHARED_LIB_ALGO)
    add_library(${PROJECT_NAME} SHARED
        ${ALGO_CORE_SRC}
        ${ALGO_CORE_HDR})
else()
    add_library(${PROJECT_NAME} STATIC
        ${ALGO_CORE_SRC}
        ${ALGO_CORE_HDR})
endif()
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)
target_include_directories(${PROJECT_NAME} PUBLIC inc)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} Eigen3::Eigen)
