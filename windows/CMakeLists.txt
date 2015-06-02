cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

target_compile_options(filter PRIVATE /wd4200 /wd4458)
target_compile_options(vis PRIVATE /wd4200 /wd4458)

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
    src/RCTrackerDemo/ "$ENV{RSSDK_DIR}/include/"
    src
)
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
  ../corvis/src/filter/
  "$ENV{RSSDK_DIR}/include"
    src
)
target_compile_definitions(RealityCapLib PRIVATE UNICODE _UNICODE)
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RealityCapLib "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")
target_link_libraries(RealityCapLib RCTracker)
target_compile_options(RealityCapLib PRIVATE /wd4200 /wd4458)

add_executable(RCUtility WIN32
#TODO: IMUManager and LocationManager appear to be unused
#      delete them if this is true
#    RCUtility/IMUManager.cpp
#    RCUtility/IMUManager.h
#    RCUtility/LocationManager.cpp
#    RCUtility/LocationManager.h
    RCUtility/RCUtility.cpp
    RCUtility/RCUtility.h
    RCUtility/RCUtility.rc
    RCUtility/Resource.h
    RCUtility/stdafx.cpp
    RCUtility/stdafx.h
    RCUtility/targetver.h
)
target_link_libraries(RCUtility RealityCapLib)
target_compile_options(RCUtility PRIVATE /wd4200 /wd4458)
target_compile_definitions(RCUtility PRIVATE UNICODE _UNICODE)
target_compile_options(RCUtility PRIVATE /ZW /EHsc /Gm- "/AIC:/Program Files (x86)/Windows Kits/8.1/References/CommonConfiguration/Neutral"
    "/AIC:/Program Files (x86)/Microsoft SDKs/Windows/v8.1/ExtensionSDKs/Microsoft.VCLibs/12.0/References/CommonConfiguration/neutral")
target_include_directories(RCUtility PRIVATE
    "$ENV{RSSDK_DIR}/include"
    RealityCapLib/
)
link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")
target_link_libraries(RCUtility "$ENV{RSSDK_DIR}lib/$(PlatformName)/libpxcmd.lib")
