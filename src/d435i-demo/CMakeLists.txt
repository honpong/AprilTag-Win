cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

set(PROJECT_NAME d435i-demo)
project(${PROJECT_NAME})

############################################################################################################
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
SET(JSON_SRC_DIR ${THIRDPARTY_DIR}/Json/src)
SET(JSON_INC_DIR ${THIRDPARTY_DIR}/Json/include)
SET(JSON_SRC "${JSON_SRC_DIR}/jsoncpp.cpp")

#############################################################################################################
include_directories(${JSON_INC_DIR})

find_package(libSP QUIET)
if(libSP_FOUND)
    add_definitions(-DlibSP_FOUND)
    set(libSP libSP)
endif()
    
FILE(GLOB ${PROJECT_NAME}_DEV_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../dev/rs_sf*.cpp")
FILE(GLOB ${PROJECT_NAME}_DEV_HDR "${CMAKE_CURRENT_SOURCE_DIR}/../dev/*.h*")
FILE(GLOB ${PROJECT_NAME}_DEMO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB ${PROJECT_NAME}_DEMO_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")
add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_DEV_SRC}
    ${${PROJECT_NAME}_DEV_HDR}
    ${${PROJECT_NAME}_DEMO_SRC}
    ${${PROJECT_NAME}_DEMO_HDR}
    ${JSON_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_sf_pose_tracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_sf_pose_tracker.h)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME} shapefit-core ${LIBRS_LIBS} ${libSP} )
target_include_directories(${PROJECT_NAME} PRIVATE shapefit-core ${LIBRS_LIBS} ${libSP}
    ${CMAKE_CURRENT_SOURCE_DIR}/../dev
    ${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk
    ${THIRDPARTY_DIR}/LibRealSense/librealsense/third-party
    ${THIRDPARTY_DIR}/librealsense/librealsense/third-party)

if(MSVC)
    if(BUILD_SHARED_LIBS)
        list(APPEND APP_DEPENDENCY "$<TARGET_FILE:realsense2>" "$<TARGET_LINKER_FILE:realsense2>")
    endif()
    if(BUILD_SHARED_LIB_ALGO)
        list(APPEND APP_DEPENDENCY "$<TARGET_FILE:shapefit-core>")
    endif()
    if(libSP_FOUND)
        list(APPEND APP_DEPENDENCY "$<TARGET_FILE:libSP>")
    endif()

    foreach(var IN LISTS APP_DEPENDENCY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(var)

    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif(MSVC)
