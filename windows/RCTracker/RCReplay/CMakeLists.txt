cmake_minimum_required(VERSION 2.8.11)
project(RCReplay)

set(SOURCES
    ../RCTrackerDemo/replay_rsclip.cpp
)

include_directories("{CMAKE_CURRENT_SOURCE_DIR}" "../../../corvis/src/filter/" "$ENV{RSSDK_DIR}/include/" "../RCTrackerDemo")

link_directories("$ENV{RSSDK_DIR}/lib/$(PlatformName)")

add_executable(${PROJECT_NAME}
    ${SOURCES}
)


target_link_libraries(${PROJECT_NAME} "../Release/RCTracker" "libpxcmd")

