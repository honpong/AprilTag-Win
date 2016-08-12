cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(qhull C)

file(GLOB QHULL_SOURCES qhull/src/libqhull/*.c)
add_library(qhull SHARED ${QHULL_SOURCES})
target_include_directories(qhull PUBLIC qhull/src/libqhull)
