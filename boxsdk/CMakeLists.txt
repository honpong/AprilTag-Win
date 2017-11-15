cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

set(BUILD_SHARED_LIB_ALGO OFF CACHE BOOL "" FORCE)
option(BUILD_SHARED_LIB_BOXSDK "Build Box SDK SHARED LIBRARY" OFF)
if(BUILD_SHARED_LIB_BOXSDK)
    add_library(box-sdk SHARED
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_camera.hpp
    	rs_box_sdk.cpp
		rs_icon.cpp)
else()
    add_library(box-sdk STATIC
        inc/rs_box_sdk.hpp
        rs_box_api.h
        rs_box_camera.hpp
    	rs_box_sdk.cpp
		rs_icon.cpp)
endif()

target_include_directories(box-sdk 
    PUBLIC  inc ${LIBRS_LIBS})
    
target_link_libraries(box-sdk 
    PUBLIC  ${LIBRS_LIBS}
    PRIVATE ${ALGO_LIB})

add_executable(box-app
    rs-box-app.cpp
    rs-box-app.hpp
	rs_icon.cpp
    ${LIBRS_DIR}/third-party/imgui/imgui.cpp
    ${LIBRS_DIR}/third-party/imgui/imgui_draw.cpp
    ${LIBRS_DIR}/third-party/imgui/imgui_impl_glfw.cpp)

include_directories(box-app
    ${LIBRS_DIR}/common
    ${LIBRS_DIR}/third-party/imgui)

target_link_libraries(box-app
    PRIVATE box-sdk)

set_property(TARGET box-sdk PROPERTY CXX_STANDARD 14)
set_property(TARGET box-app PROPERTY CXX_STANDARD 14)

if(MSVC)
    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${ALGO_LIB}>" "$<TARGET_FILE_DIR:box-app>")
    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:realsense2>"     "$<TARGET_FILE_DIR:box-app>")
    add_custom_command(TARGET box-app POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:box-sdk>"        "$<TARGET_FILE_DIR:box-app>")
#	add_custom_command(TARGET shapefit-dev POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBREALSENSE_BIN_PATH}/${LIBREALSENSE_LIB}.dll" "$<TARGET_FILE_DIR:shapefit-dev>")
	set_target_properties(box-app PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif()
