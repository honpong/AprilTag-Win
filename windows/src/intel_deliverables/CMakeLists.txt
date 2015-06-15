cmake_minimum_required(VERSION 2.8.11)

project(RCUtility)

add_subdirectory(ThirdParty "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty")

add_library(pxcmd STATIC IMPORTED)
set_property(TARGET pxcmd PROPERTY IMPORTED_LOCATION "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")

add_library(RealityCapLib
  RealityCapLib/TrackerManager.cpp
  RealityCapLib/SensorManager.cpp
  RealityCapLib/RCFactory.cpp
  RealityCapLib/Debug.cpp
  RealityCapLib/stdafx.cpp
)
target_include_directories(RealityCapLib PUBLIC RealityCapLib)
target_include_directories(RealityCapLib PUBLIC 
  RCTracker
  "$ENV{RSSDK_DIR}/include"
)
target_compile_definitions(RealityCapLib PRIVATE UNICODE _UNICODE)
link_directories("RCTracker")
target_link_libraries(RealityCapLib RCTracker pxcmd)
target_compile_options(RealityCapLib PRIVATE /wd4200 /wd4458)

add_executable(RCTrackerDemo
    RCTrackerDemo/replay_rsclip.cpp
    )
target_link_libraries(RCTrackerDemo
    RealityCapLib
    RCTracker)

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
target_link_libraries(RCUtility RealityCapLib pxcmd)
target_link_libraries(RCUtility glad glfw)
target_link_libraries(RCUtility ${GLFW_LIBRARIES})
target_compile_options(RCUtility PRIVATE /wd4200 /wd4458)
target_compile_definitions(RCUtility PRIVATE UNICODE _UNICODE)
target_include_directories(RCUtility PRIVATE "$ENV{RSSDK_DIR}/include")

add_custom_command(TARGET RCUtility POST_BUILD        # Adds a post-build event to RCUtility
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/RCTracker/RCTracker.dll"      # <--this is in-file
        $<TARGET_FILE_DIR:RCUtility>)                 # <--this is out-file path

add_custom_command(TARGET RCUtility POST_BUILD        # Adds a post-build event to RCUtility
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/RCUtility/gigabyte_s11.json"      # <--this is in-file
        $<TARGET_FILE_DIR:RCUtility>)                 # <--this is out-file path

