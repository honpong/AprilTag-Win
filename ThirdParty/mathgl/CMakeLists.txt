cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(mgl)

set(enable-rvalue ON CACHE BOOL "")
set(enable-lgpl ON CACHE BOOL "")
set(enable-png OFF CACHE BOOL "")
set(enable-openmp OFF CACHE BOOL "")
set(enable-pthread OFF CACHE BOOL "")
set(enable-opengl OFF CACHE BOOL "")
set(enable-mpi OFF CACHE BOOL "")
set(enable-zlib OFF CACHE BOOL "")
set(enable-all-widgets OFF CACHE BOOL "")
add_subdirectory(mathgl)
set_target_properties(mgl PROPERTIES MACOSX_RPATH ON)
target_include_directories(mgl INTERFACE mathgl/include $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/mathgl/include>)
set_target_properties(mgl_example mgl-static PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
target_compile_options(mgl PUBLIC    $<$<CXX_COMPILER_ID:Intel>: -diag-disable=3180> # unrecognized OpenMP #pragma
                                     $<$<CXX_COMPILER_ID:Clang>: -Wno-deprecated-register>
                                     $<$<CXX_COMPILER_ID:AppleClang>: -Wno-deprecated-register>)
target_compile_options(mgl PRIVATE   $<$<CXX_COMPILER_ID:Clang>: -Wno-deprecated-declarations>) # internal to mathgl which requires cmake < 3.0 which didn't have AppleCLang
target_compile_options(mgl INTERFACE $<$<CXX_COMPILER_ID:AppleClang>: -Wno-return-type-c-linkage -Wno-parentheses-equality>)
