cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

project(shapefit-dev)

############################################################################################################
SET(THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
SET(JSON_SRC_DIR ${THIRDPARTY_DIR}/Json/src)
SET(JSON_INC_DIR ${THIRDPARTY_DIR}/Json/include)
SET(JSON_SRC "${JSON_SRC_DIR}/jsoncpp.cpp")

#############################################################################################################
include_directories(${JSON_INC_DIR})
    
FILE(GLOB SHAPEFIT_DEV_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB SHAPEFIT_DEV_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*" )
add_executable(shapefit-dev
    ${SHAPEFIT_DEV_SRC}
	${SHAPEFIT_DEV_HDR}
    ${JSON_SRC})
set_property(TARGET shapefit-dev PROPERTY CXX_STANDARD 14)
target_link_libraries(shapefit-dev shapefit-core ${LIBRS_LIBS})
target_include_directories(shapefit-dev PRIVATE shapefit-core ${LIBRS_LIBS})

IF(WIN32)
	add_custom_command(TARGET shapefit-dev POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:shapefit-core>" "$<TARGET_FILE_DIR:shapefit-dev>")
#	add_custom_command(TARGET shapefit-dev POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBREALSENSE_BIN_PATH}/${LIBREALSENSE_LIB}.dll" "$<TARGET_FILE_DIR:shapefit-dev>")
	set_target_properties(shapefit-dev PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir) ")
ENDIF()
