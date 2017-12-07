cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

set(PROJECT_NAME shapefit-dev)
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
endif()
    
FILE(GLOB SHAPEFIT_DEV_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB SHAPEFIT_DEV_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*" )
add_executable(shapefit-dev
    ${SHAPEFIT_DEV_SRC}
	${SHAPEFIT_DEV_HDR}
    ${JSON_SRC})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME} shapefit-core ${LIBRS_LIBS} ${libSP_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE shapefit-core ${LIBRS_LIBS} ${libSP_INCLUDE_DIR})

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
