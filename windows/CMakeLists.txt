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


add_library(RealityCapLib
  RealityCapLib/CalibrationManager.cpp
  RealityCapLib/CaptureManager.cpp
  RealityCapLib/SensorData.cpp
  RealityCapLib/SensorManager.cpp
  RealityCapLib/RCFactory.cpp
  RealityCapLib/Debug.cpp
  RealityCapLib/stdafx.cpp
)
target_include_directories(RealityCapLib PRIVATE 
  ../corvis/src/filter                                       
  "$ENV{RSSDK_DIR}/include"
)
target_compile_definitions(RealityCapLib PRIVATE UNICODE _UNICODE)
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RealityCapLib "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")

add_executable(RCUtility RCUtility/RCUtility.cpp
RCUtility/LocationManager.cpp
../corvis/src/filter/rc_intel_interface.cpp
)
#target_link_libraries(RCUtility RCTracker)
target_link_libraries(RCUtility
   cor
   numerics
   filter
   vis
   casablanca
   RealityCapLib
)
target_compile_definitions(RCUtility PRIVATE UNICODE _UNICODE)
target_compile_options(RCUtility PRIVATE /ZW /EHsc /Gm- "/AIC:/Program Files (x86)/Windows Kits/8.1/References/CommonConfiguration/Neutral"
    "/AIC:/Program Files (x86)/Microsoft SDKs/Windows/v8.1/ExtensionSDKs/Microsoft.VCLibs/12.0/References/CommonConfiguration/neutral")
target_include_directories(RCUtility PRIVATE ../corvis/src/filter
    src/RCUtility "$ENV{RSSDK_DIR}/include"
    RealityCapLib/
    ../corvis/src/
    ../corvis/src/vis
)
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RCUtility "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")
