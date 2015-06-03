cmake_minimum_required(VERSION 2.8.11)
project(RCUtility)

add_definitions(-DUNICODE -D_UNICODE)

set(SOURCES
    RCUtility.cpp
)

include_directories("{CMAKE_CURRENT_SOURCE_DIR}" "$ENV{RSSDK_DIR}include/")

link_directories("$ENV{RSSDK_DIR}lib/$(PlatformName)")

add_executable(${PROJECT_NAME}
    ${SOURCES}
)

target_link_libraries(${PROJECT_NAME} "RCTracker" "RealityCapLib")

