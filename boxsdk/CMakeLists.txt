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

find_package(libSP QUIET)
if(libSP_FOUND)
    add_definitions(-DlibSP_FOUND)
endif()

target_include_directories(box-sdk 
    PUBLIC  inc ${LIBRS_LIBS} 
	PRIVATE libSP)
    
target_link_libraries(box-sdk 
    PUBLIC  ${LIBRS_LIBS} libSP
    PRIVATE ${ALGO_LIB})

add_executable(box-app
    rs-box-app.cpp
    rs-box-app.hpp
	rs_icon.h)

target_link_libraries(box-app
    PRIVATE box-sdk)

set_property(TARGET box-sdk PROPERTY CXX_STANDARD 14)
set_property(TARGET box-app PROPERTY CXX_STANDARD 14)

if(MSVC)
	if(BUILD_SHARED_LIBS)
		list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:realsense2>" "$<TARGET_LINKER_FILE:realsense2>")
    endif()
	if(libSP_FOUND)
		list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:libSP>")
    endif()
	if(BUILD_SHARED_LIB_BOXSDK)
		list(APPEND BOX_SDK_DEPENDENCY "$<TARGET_FILE:box-sdk>" "$<TARGET_LINKER_FILE:box-sdk>")
	endif()

	foreach(var IN LISTS BOX_SDK_DEPENDENCY)
		if(BUILD_SHARED_LIB_BOXSDK)
		    add_custom_command(TARGET box-sdk POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:box-sdk>")
		else()
			add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:box-app>")
		endif()
	endforeach(var)
	set_target_properties(box-app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif()
