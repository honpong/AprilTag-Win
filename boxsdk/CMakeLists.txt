cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

set(BUILD_SHARED_LIB_ALGO OFF CACHE BOOL "" FORCE)
option(BUILD_SHARED_LIB_BOXSDK "Build Box SDK SHARED LIBRARY" OFF)
if(BUILD_SHARED_LIB_BOXSDK)
    add_library(box-sdk SHARED
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_image.hpp
    	rs_box_sdk.cpp
        rs_icon.h
        rs_sf_pose_tracker.h
        rs_sf_pose_tracker.cpp)
else()
    add_library(box-sdk STATIC
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_image.hpp
    	rs_box_sdk.cpp
	rs_icon.h
        rs_sf_pose_tracker.h
        rs_sf_pose_tracker.cpp)
endif()

find_package(libSP QUIET PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../src/thirdparty/sp/win")
if(libSP_FOUND)
    add_definitions(-DlibSP_FOUND)
endif()

target_include_directories(box-sdk 
    PUBLIC  inc ${LIBRS_LIBS} ${libSP_INCLUDE_DIR})
    
link_directories(${libSP_LIB_DIR})
target_link_libraries(box-sdk 
    PUBLIC  ${LIBRS_LIBS}
    PRIVATE ${ALGO_LIB} ${libSP_LIB})

add_executable(box-app
    rs-box-app.cpp
    rs-box-app.hpp
	rs_icon.h)

include_directories(box-app
    ${LIBRS_DIR}/common
    ${LIBRS_DIR}/third-party/imgui)

target_link_libraries(box-app
    PRIVATE box-sdk)

set_property(TARGET box-sdk PROPERTY CXX_STANDARD 14)
set_property(TARGET box-app PROPERTY CXX_STANDARD 14)

if(MSVC)
	if(libSP_FOUND)
	    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libSP_BIN_DIR}/RCTracker.dll" "$<TARGET_FILE_DIR:box-app>")
        add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${libSP_BIN_DIR}/SP_Core.dll"   "$<TARGET_FILE_DIR:box-app>")
    endif()
	add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${ALGO_LIB}>" "$<TARGET_FILE_DIR:box-app>")
    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:realsense2>"  "$<TARGET_FILE_DIR:box-app>")
    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:box-sdk>"     "$<TARGET_FILE_DIR:box-app>")
	set_target_properties(box-app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif()
