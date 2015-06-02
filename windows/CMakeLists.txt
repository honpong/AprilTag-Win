cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

add_library(RCTracker SHARED ../corvis/src/filter/rc_intel_interface.cpp)
target_link_libraries(RCTracker PRIVATE
  filter
  casablanca
  numerics
  cor
  vis
)
set_property(TARGET RCTracker PROPERTY CXX_STANDARD 14)

add_executable(RCTrackerDemo src/RCTrackerDemo/replay_rsclip.cpp)
target_link_libraries(RCTrackerDemo RCTracker)
target_include_directories(RCTrackerDemo PRIVATE ../corvis/src/filter/
    src/RCTrackerDemo/ "$ENV{RSSDK_DIR}/include/")
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RCTrackerDemo "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")


add_executable(RCUtility RCUtility/RCUtility.cpp RCUtility/LocationManager.cpp)
# target_link_libraries(RCUtility RCTracker)
target_include_directories(RCUtility PRIVATE ../corvis/src/filter
    src/RCUtility "$ENV{RSSDK_DIR}/include")
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RCUtility "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")
