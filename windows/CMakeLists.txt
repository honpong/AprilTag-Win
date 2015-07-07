cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(RealityCap)

# /arch:sse2 is implied on 64 bit, and visual studio complains if you specify it
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	add_compile_options(/openmp)
else( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	add_compile_options(/arch:SSE2 /openmp)
endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

add_library(pxc STATIC IMPORTED)
set_target_properties(pxc PROPERTIES
  IMPORTED_LOCATION_DEBUG       "$ENV{RSSDK_DIR}/lib/$(PlatformName)/libpxc_d.lib"
  IMPORTED_LOCATION_RELEASE     "$ENV{RSSDK_DIR}/lib/$(PlatformName)/libpxc.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "$ENV{RSSDK_DIR}/lib/$(PlatformName)/libpxc.lib"
  IMPORTED_LOCATION_MINSIZEREL  "$ENV{RSSDK_DIR}/lib/$(PlatformName)/libpxc.lib"
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{RSSDK_DIR}/include"
)

target_compile_options(filter PRIVATE /wd4200 /wd4458)
target_compile_options(vis PRIVATE /wd4200 /wd4458)

add_library(RCTracker SHARED
    ../corvis/src/filter/rc_intel_interface.cpp
    ../corvis/src/filter/rc_intel_interface.h
)

target_link_libraries(RCTracker PRIVATE
  filter
  numerics
  cor
  vis
)
set_property(TARGET RCTracker PROPERTY CXX_STANDARD 14)
target_include_directories(RCTracker PUBLIC
    ../corvis/src/filter
)

add_executable(RCTrackerDemo src/RCTrackerDemo/replay_rsclip.cpp)
target_link_libraries(RCTrackerDemo RealityCapLib)

add_custom_command(TARGET RCTrackerDemo POST_BUILD        # Adds a post-build event to RCTrackerDemo
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/src/intel_deliverables/gigabyte_s11.json"      # <--this is in-file
        $<TARGET_FILE_DIR:RCTrackerDemo>)                 # <--this is out-file path

add_custom_command(TARGET RCTrackerDemo POST_BUILD        # Adds a post-build event to RCTrackerDemo
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/src/RCTrackerDemo/convert_clip.bat"      # <--this is in-file
        $<TARGET_FILE_DIR:RCTrackerDemo>)                 # <--this is out-file path

add_library(RealityCapLib
  RealityCapLib/TrackerManager.cpp
  RealityCapLib/TrackerManager.h
  RealityCapLib/SensorManager.cpp
  RealityCapLib/SensorManager.h
  RealityCapLib/RCFactory.cpp
  RealityCapLib/RCFactory.h
  RealityCapLib/Debug.cpp
  RealityCapLib/Debug.h
  RealityCapLib/stdafx.cpp
  RealityCapLib/stdafx.h
  RealityCapLib/rc_pxc_util.h
)
target_include_directories(RealityCapLib PUBLIC RealityCapLib)
target_compile_definitions(RealityCapLib PRIVATE UNICODE _UNICODE)
target_link_libraries(RealityCapLib PUBLIC RCTracker pxc)
target_compile_options(RealityCapLib PRIVATE /wd4200 /wd4458)

add_executable(RCUtility WIN32
    RCUtility/RCUtility.cpp
    RCUtility/RCUtility.h
    RCUtility/RCUtility.rc
    RCUtility/Resource.h
    RCUtility/stdafx.cpp
    RCUtility/stdafx.h
    RCUtility/targetver.h
    RCUtility/render_data.cpp
    RCUtility/render_data.h
    RCUtility/arcball.cpp
    RCUtility/arcball.h
    RCUtility/visualization.cpp
    RCUtility/visualization.h
    RCUtility/FilePicker.h
)

target_compile_options(glfw PRIVATE /wd4457)

target_link_libraries(RCUtility RealityCapLib)
target_link_libraries(RCUtility glad glfw)
target_link_libraries(RCUtility ${GLFW_LIBRARIES})
target_compile_options(RCUtility PRIVATE /wd4200 /wd4458)
target_compile_definitions(RCUtility PRIVATE UNICODE _UNICODE)

add_custom_command(TARGET RCUtility POST_BUILD        # Adds a post-build event to RCUtility
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        "${PROJECT_SOURCE_DIR}/src/intel_deliverables/gigabyte_s11.json"      # <--this is in-file
        $<TARGET_FILE_DIR:RCUtility>)                 # <--this is out-file path

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist/ThirdParty)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist/RCTracker)
file(COPY RCUtility DESTINATION dist)
file(COPY src/RCTrackerDemo DESTINATION dist)
file(COPY RealityCapLib DESTINATION dist)
file(COPY src/intel_deliverables/CMakeLists.txt DESTINATION dist)
file(COPY ../corvis/src/filter/rc_intel_interface.h DESTINATION dist/RCTracker)
file(COPY ../corvis/src/glfw DESTINATION dist/ThirdParty)
file(COPY src/intel_deliverables/ThirdParty/CMakeLists.txt DESTINATION dist/ThirdParty)
file(COPY src/intel_deliverables/gigabyte_s11.json DESTINATION dist/RCUtility)

# Custom targets will always be rebuilt, which means this target will
# copy the updated the DLL whenever it is built
add_custom_target(
    BuildIntelDistribution ALL
    DEPENDS RCTracker)
add_custom_command(TARGET BuildIntelDistribution POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RCTracker> dist/RCTracker/)
add_custom_command(TARGET BuildIntelDistribution POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy $<TARGET_LINKER_FILE:RCTracker> dist/RCTracker/)
add_dependencies(RCUtility BuildIntelDistribution)
add_dependencies(RCTrackerDemo BuildIntelDistribution)

target_compile_options(glfw PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(cor PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(filter PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(glad PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(numerics PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(measure PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(RCTracker PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(vis PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(spec PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
target_compile_options(spec_intel PRIVATE "/MT$<$<STREQUAL:$<CONFIGURATION>,Debug>:d>")
