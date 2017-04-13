cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

project(shapefit_api C CXX)

#############################################################################################
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
SET(EIGEN_DIR ${THIRDPARTY_DIR}/eigen)
SET(SHAPEFIT_API_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc CACHE STRING "")
SET(OpenCV_STATIC ON)
find_package(OPENCV QUIET)
if(OPENCV_FOUND OR OpenCV_FOUND)
	add_definitions(-DOPENCV_FOUND)
endif()

#############################################################################################
include_directories(
    ${OPENCV_INCLUDE_DIR} 
    ${EIGEN_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc)
    
FILE(GLOB SHAPEFIT_API_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB SHAPEFIT_API_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${SHAPEFIT_API_HDR_DIR}/*")
option(BUILD_SHAPEFIT_LIB_SHARED "Build Shared Shapefit Library" ON)
if(BUILD_SHAPEFIT_LIB_SHARED)
    add_library(shapefit_api SHARED 
        ${SHAPEFIT_API_SRC}
        ${SHAPEFIT_API_HDR})
else()
    add_library(shapefit_api STATIC 
        ${SHAPEFIT_API_SRC}
        ${SHAPEFIT_API_HDR})
endif()
set_property(TARGET shapefit_api PROPERTY CXX_STANDARD 14)
target_include_directories(shapefit_api PUBLIC inc)
target_link_libraries(shapefit_api PRIVATE ${OpenCV_LIBS})
