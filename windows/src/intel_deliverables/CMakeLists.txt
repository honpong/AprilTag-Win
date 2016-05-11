cmake_minimum_required(VERSION 2.8.11)

project(RCUtility)

add_subdirectory(ThirdParty "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty")

add_library(pxcmd STATIC IMPORTED)
set_target_properties(pxcmd PROPERTIES
  IMPORTED_LOCATION             "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib"
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{RSSDK_DIR}include"
)

add_library(RCTracker SHARED IMPORTED)
set_target_properties(RCTracker PROPERTIES
  IMPORTED_IMPLIB   "${PROJECT_SOURCE_DIR}/RCTracker/RCTracker.lib"
  IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/RCTracker/RCTracker.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/RCTracker"
)

add_library(RealityCapLib
  RealityCapLib/TrackerManager.cpp
  RealityCapLib/SensorManager.cpp
  RealityCapLib/RCFactory.cpp
  RealityCapLib/Debug.cpp
  RealityCapLib/stdafx.cpp
)
target_include_directories(RealityCapLib PUBLIC RealityCapLib)
target_compile_definitions(RealityCapLib PRIVATE UNICODE _UNICODE)
target_link_libraries(RealityCapLib PUBLIC RCTracker pxcmd)
target_compile_options(RealityCapLib PRIVATE /wd4200 /wd4458)

add_executable(RCTrackerDemo
    RCTrackerDemo/replay_rsclip.cpp
    )
target_link_libraries(RCTrackerDemo RealityCapLib)

add_executable(rc_replay
    rc_replay/rc_replay.cpp
    rc_replay/main.cpp
)
set_target_properties(rc_replay PROPERTIES CXX_STANDARD 14)
target_link_libraries(rc_replay RCTracker)

add_executable(RCUtility WIN32
    RCUtility/RCUtility.cpp
    RCUtility/RCUtility.h
    RCUtility/RCUtility.rc
    RCUtility/Resource.h
    RCUtility/stdafx.cpp
    RCUtility/stdafx.h
    RCUtility/targetver.h
    RCUtility/render_data.cpp
    RCUtility/arcball.cpp
    RCUtility/visualization.cpp
    RCUtility/FilePicker.h
)
target_link_libraries(RCUtility RealityCapLib)
target_link_libraries(RCUtility glad glfw)
target_link_libraries(RCUtility ${GLFW_LIBRARIES})
target_compile_options(RCUtility PRIVATE /wd4200 /wd4458)
target_compile_definitions(RCUtility PRIVATE UNICODE _UNICODE)

add_custom_command(TARGET RCUtility POST_BUILD        # Adds a post-build event to RCUtility
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:RCTracker>                      # <--this is in-file
        $<TARGET_FILE_DIR:RCUtility>)                 # <--this is out-file path

add_custom_command(TARGET RCUtility POST_BUILD        # Adds a post-build event to RCUtility
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/RCUtility/gigabyte_s11.json"      # <--this is in-file
        $<TARGET_FILE_DIR:RCUtility>)                 # <--this is out-file path

