cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

#############################################################################################
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
SET(EIGEN_DIR ${THIRDPARTY_DIR}/eigen)
SET(SHAPEFIT_CORE_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc CACHE STRING "")
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
    
FILE(GLOB SHAPEFIT_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB SHAPEFIT_CORE_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${SHAPEFIT_CORE_HDR_DIR}/*")
option(BUILD_SHARED_LIB_SHAPEFIT "Build Shared Shapefit Library" ON)
if(BUILD_SHARED_LIB_SHAPEFIT)
    add_library(shapefit-core SHARED 
        ${SHAPEFIT_CORE_SRC}
        ${SHAPEFIT_CORE_HDR})
else()
    add_library(shapefit-core STATIC 
        ${SHAPEFIT_CORE_SRC}
        ${SHAPEFIT_CORE_HDR})
endif()
set_property(TARGET shapefit-core PROPERTY CXX_STANDARD 14)
set_target_properties (shapefit-core PROPERTIES FOLDER Library)
target_include_directories(shapefit-core PUBLIC inc)
target_link_libraries(shapefit-core PRIVATE ${OpenCV_LIBS})
