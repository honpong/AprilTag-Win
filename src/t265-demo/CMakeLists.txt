cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

set(PROJECT_NAME t265-demo)
project(${PROJECT_NAME})

############################################################################################################
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
SET(JSON_SRC_DIR ${THIRDPARTY_DIR}/Json/src)
SET(JSON_INC_DIR ${THIRDPARTY_DIR}/Json/include)
SET(JSON_SRC "${JSON_SRC_DIR}/jsoncpp.cpp")

#############################################################################################################
include_directories(${JSON_INC_DIR})

#find_package(libSP QUIET)
if(libSP_FOUND)
    add_definitions(-DlibSP_FOUND)
    set(libSP libSP)
endif()

find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    add_definitions(-DOpenCV_FOUND)
    message(STATUS "OpenCV package is added to t265-demo")
endif()

#FILE(GLOB ${PROJECT_NAME}_DEV_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../dev/rs_sf*.cpp")
#FILE(GLOB ${PROJECT_NAME}_DEV_HDR "${CMAKE_CURRENT_SOURCE_DIR}/../dev/*.h*")
FILE(GLOB ${PROJECT_NAME}_DEMO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB ${PROJECT_NAME}_DEMO_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")
FILE(GLOB ${PROJECT_NAME}_DEMO_BAT "${CMAKE_CURRENT_SOURCE_DIR}/*.bat")
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_DEV_SRC}
    ${${PROJECT_NAME}_DEV_HDR}
    ${${PROJECT_NAME}_DEMO_SRC}
    ${${PROJECT_NAME}_DEMO_HDR}
	${${PROJECT_NAME}_DEMO_BAT}
    ${JSON_SRC}
    #${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_sf_pose_tracker.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_sf_pose_tracker.h
    #${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_icon.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk/rs_icon.h
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRS_LIBS} ${libSP} ${OpenCV_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRS_LIBS} ${libSP} ${OpenCV_DIR}
    #${CMAKE_CURRENT_SOURCE_DIR}/../dev
    #${CMAKE_CURRENT_SOURCE_DIR}/../../boxsdk
    ${THIRDPARTY_DIR}/LibRealSense/librealsense/third-party
    ${THIRDPARTY_DIR}/librealsense/librealsense/third-party
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/MacOSXBundleInfo.plist.in"
)

if(MSVC)
    if(BUILD_SHARED_LIBS)
        list(APPEND APP_DEPENDENCY "$<TARGET_FILE:realsense2>" "$<TARGET_LINKER_FILE:realsense2>")
    endif()
    #if(BUILD_SHARED_LIB_ALGO)
    #    list(APPEND APP_DEPENDENCY "$<TARGET_FILE:shapefit-core>")
    #endif()
    #if(libSP_FOUND)
    #    list(APPEND APP_DEPENDENCY "$<TARGET_FILE:libSP>")
    #endif()
    #if(TARGET tracker)
    #    list(APPEND APP_DEPENDENCY "$<TARGET_FILE:tracker>" "$<TARGET_FILE:vocabulary>")
    #endif()

	if(OpenCV_FOUND)
	    list(APPEND APP_DEPENDENCY ${OpenCV_LIB_DIR}/../bin/opencv_world310$<$<CONFIG:Debug>:d>.dll)
	endif()

    foreach(var IN LISTS APP_DEPENDENCY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(var)
    
	foreach(python_script translatev1 csvrwv1)
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND pyinstaller --noconfirm --onefile --distpath ${CMAKE_CURRENT_BINARY_DIR}/python_binary --workpath ${CMAKE_CURRENT_BINARY_DIR}/python_temp ${CMAKE_CURRENT_SOURCE_DIR}/${python_script}.py)
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/python_binary/${python_script}.exe "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endforeach(python_script)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/capture")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/t265-insight.bat "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
endif(MSVC)
