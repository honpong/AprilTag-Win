cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(Tracker)

add_compile_options(-fvisibility=hidden)

# Match the options used by default in android-ndk-*/toolchains/*-clang3.6/setup.mk
add_compile_options(-ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -fno-omit-frame-pointer)
set(CMAKE_POSITION_INDEPENDENT_CODE True)

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

add_library(tracker SHARED ../corvis/src/filter/rc_intel_interface.cpp)
set_target_properties(tracker PROPERTIES CXX_STANDARD 14)
target_link_libraries(tracker filter numerics cor)

set_target_properties(measure spec spec_intel PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
