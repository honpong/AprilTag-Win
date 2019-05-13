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

    set(InsightSDK_DIR ${OpenCV_DIR}/../../InsightSDK/)
endif()

if(NOT SPINNAKER_DIR)
    set(SPINNAKER_DIR "C:/Program Files/Point Grey Research/Spinnaker")
endif()

message(STATUS "SPINNAKER_DIR :: ${SPINNAKER_DIR} ${CMAKE_GENERATOR_PLATFORM} ${CMAKE_VS_PLATFORM_NAME}")

if(${CMAKE_VS_PLATFORM_NAME} STREQUAL x64)
   set(PLATFORM_SUFFIX "64")
   set(DEP_FOLDER "Win64_x64")
else()
   set(PLATFORM_SUFFIX "")
   set(DEP_FOLDER "Win32_i86")
endif()
set(SPINNAKER_INCLUDE_DIR ${SPINNAKER_DIR}/include)
set(SPINNAKER_LIB_DIR "${SPINNAKER_DIR}/lib${PLATFORM_SUFFIX}/vs2015"
                      "${SPINNAKER_DIR}/lib${PLATFORM_SUFFIX}"
                      )
set(SPINNAKER_LIB     Spinnaker$<$<CONFIG:Debug>:d>_v140)
set(SPINNAKER_BIN_DIR "${SPINNAKER_DIR}/bin${PLATFORM_SUFFIX}/vs2015")
set(SPINNAKER_DEP_DIR "${SPINNAKER_DIR}/dependencies/GenICam_v3_0/bin/${DEP_FOLDER}/msvc2015")
message(STATUS "SPINNAKER_LIB_DIR ${SPINNAKER_LIB_DIR}")
message(STATUS "SPINNAKER_DEP_DIR ${SPINNAKER_DEP_DIR}")
link_directories(${SPINNAKER_LIB_DIR})


#FILE(GLOB ${PROJECT_NAME}_DEV_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../dev/rs_sf*.cpp")
#FILE(GLOB ${PROJECT_NAME}_DEV_HDR "${CMAKE_CURRENT_SOURCE_DIR}/../dev/*.h*")
FILE(GLOB ${PROJECT_NAME}_DEMO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
FILE(GLOB ${PROJECT_NAME}_DEMO_HDR "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")
FILE(GLOB ${PROJECT_NAME}_DEMO_BAT "${CMAKE_CURRENT_SOURCE_DIR}/*.bat")
FILE(GLOB ${PROJECT_NAME}_DEMO_PY  "${CMAKE_CURRENT_SOURCE_DIR}/*.py")

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_DEV_SRC}
    ${${PROJECT_NAME}_DEV_HDR}
    ${${PROJECT_NAME}_DEMO_SRC}
    ${${PROJECT_NAME}_DEMO_HDR}
	${${PROJECT_NAME}_DEMO_BAT}
    ${${PROJECT_NAME}_DEMO_PY}
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

target_link_libraries(${PROJECT_NAME} PRIVATE ${SPINNAKER_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${SPINNAKER_INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/MacOSXBundleInfo.plist.in"
)

if(UNIX)
    if(BUILD_SHARED_LIBS)
        list(APPEND APP_DEPENDENCY "$<TARGET_FILE:realsense2>" "$<TARGET_LINKER_FILE:realsense2>")
    endif()

    foreach(var IN LISTS APP_DEPENDENCY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(var)

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/capture")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/t265-insight.sh "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    
	foreach(python_script calibration_make translatev1 csvrwv1)
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${python_script}.py "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
	    #add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND pyinstaller --noconfirm --onefile --distpath ${CMAKE_CURRENT_BINARY_DIR}/python_binary --workpath ${CMAKE_CURRENT_BINARY_DIR}/python_temp ${CMAKE_CURRENT_SOURCE_DIR}/${python_script}.py)
	    #add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/python_binary/${python_script} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endforeach(python_script)

endif() #UNIX

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

    if(TARGET PTHREADS_LIBRARY)
        list(APPEND APP_DEPENDENCY $<TARGET_FILE:PTHREADS_LIBRARY>)
    endif()

    if(SPINNAKER_DIR)
        FILE(GLOB SPINNAKER_DEP_LIB "${SPINNAKER_DEP_DIR}/*.dll")
        FILE(GLOB SPINNAKER_BIN "${SPINNAKER_BIN_DIR}/*.dll")
        message(STATUS "SPINNAKER_BIN ${SPINNAKER_BIN_DIR}")
        foreach(dll IN LISTS SPINNAKER_DEP_LIB)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dll} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach(dll)
    endif()

    foreach(var IN LISTS APP_DEPENDENCY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${var} "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach(var)

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/capture")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/InsightSDK")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/t265-insight.bat "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${THIRDPARTY_DIR}/exempi/build/vc14/x64/exempi.dll "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${THIRDPARTY_DIR}/exempi/build/vc14/x64/exempi.dll "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${InsightSDK_DIR}/jar/InsightService-1.0-SNAPSHOT-20190412.jar "$<TARGET_FILE_DIR:${PROJECT_NAME}>/InsightSDK/")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory    ${InsightSDK_DIR}/jar/jdk-11.0.2 "$<TARGET_FILE_DIR:${PROJECT_NAME}>/InsightSDK/jdk-11.0.2")

    

	foreach(python_script calibration_make translatev1 csvrwv1 )
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${python_script}.py "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND pyinstaller --noconfirm --onefile --distpath ${CMAKE_CURRENT_BINARY_DIR}/python_binary --workpath ${CMAKE_CURRENT_BINARY_DIR}/python_temp ${CMAKE_CURRENT_SOURCE_DIR}/${python_script}.py)
	    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/python_binary/${python_script}.exe "$<TARGET_FILE_DIR:${PROJECT_NAME}>/py")
	endforeach(python_script)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND calibration_make.exe ../python_temp WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python_binary)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/python_temp/calibration.txt "$<TARGET_FILE_DIR:${PROJECT_NAME}>/capture")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/python_temp/calibration.txt)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND POWERSHELL rm -Force "$<TARGET_FILE_DIR:${PROJECT_NAME}>/*_MD$<$<CONFIG:Release>:d>_*.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND POWERSHELL rm -Force "$<TARGET_FILE_DIR:${PROJECT_NAME}>/CLSer*$<$<CONFIG:Release>:d>.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND POWERSHELL rm -Force "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.lib")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND POWERSHELL rm -Force "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.exp")

    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(TargetDir)")
endif(MSVC)
