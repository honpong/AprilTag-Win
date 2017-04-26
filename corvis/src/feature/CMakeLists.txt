cmake_minimum_required(VERSION 2.8)

project(feature C CXX)

set(DETECTOR_SOURCES
    detector/fast.h
    detector/fast_9.cpp
)

set(DESCRIPTOR_SOURCES
    descriptor/orb_descriptor.h
    descriptor/orb_descriptor.cpp
)

set(TRACKER_SOURCES
    tracker/fast_constants.h
    tracker/fast_tracker.h
    tracker/fast_tracker.cpp
    tracker/feature_tracker.cpp
    tracker/scaled_mask.h
    tracker/scaled_mask.cpp
    tracker/tracker.h
)

add_library(${PROJECT_NAME} STATIC ${DETECTOR_SOURCES} ${DESCRIPTOR_SOURCES} ${TRACKER_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/detector
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/descriptor
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tracker
)
