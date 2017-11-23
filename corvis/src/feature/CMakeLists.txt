cmake_minimum_required(VERSION 2.8)

project(feature CXX)

add_library(${PROJECT_NAME} STATIC
    detector/fast_9.h

    descriptor/orb_descriptor.h
    descriptor/orb_descriptor.cpp
    descriptor/patch_descriptor.h
    descriptor/patch_descriptor.cpp
    descriptor/popcount.h

    tracker/fast_constants.h
    tracker/fast_tracker.h
    tracker/fast_tracker.cpp
    tracker/feature_tracker.cpp
    tracker/scaled_mask.h
    tracker/scaled_mask.cpp
    tracker/tracker.h
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
target_include_directories(${PROJECT_NAME}
    PUBLIC detector
    PUBLIC descriptor
    PUBLIC tracker
)
