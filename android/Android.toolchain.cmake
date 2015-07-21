cmake_minimum_required(VERSION 3.2.2)

set(CMAKE_SYSTEM_NAME Linux)

if (NOT CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN AND NOT CMAKE_C_COMPILER_TARGET) # When called by try_compile these are passed in from below
  set(ANDROID_NDK_ROOT "$ENV{ANDROID_NDK_ROOT}" CACHE PATH "Android NDK Path")
  set(ANDROID_PLATFORM "android-21" CACHE STRING "Android Platform")
  set_property(CACHE ANDROID_PLATFORM PROPERTY STRINGS android-21 android-20 android-19)
  set(ANDROID_ARCH "${ANDROID_ARCH}" CACHE STRING "Android Architecture")
  set_property(CACHE ANDROID_ARCH PROPERTY STRINGS x86 x86_64 arm64)

  if(ANDROID_ARCH STREQUAL "x86")
    set(CMAKE_SYSTEM_PROCESSOR i686)
  else()
    set(CMAKE_SYSTEM_PROCESSOR ${ANDROID_ARCH})
  endif()
  set(ANDROID_TOOLCHAIN_MACHINE_NAME "${CMAKE_SYSTEM_PROCESSOR}-linux-android" CACHE INTERNAL "Toolchain target")

  set(ANDROID_TOOLCHAIN_ROOT "${CMAKE_BINARY_DIR}/toolchain" CACHE PATH "Toolchain root")
  if (NOT EXISTS "${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
    file(REMOVE_RECURSE "${ANDROID_TOOLCHAIN_ROOT}")
    execute_process(COMMAND "${ANDROID_NDK_ROOT}/build/tools/make-standalone-toolchain.sh"
                            --platform=${ANDROID_PLATFORM} --arch=${ANDROID_ARCH} --llvm-version=3.6 --stl=libcxx
                            --install-dir="${ANDROID_TOOLCHAIN_ROOT}")
    if (NOT EXISTS "${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
      message(ERROR "failed to build the toolchain: ${ANDROID_TOOLCHAIN_ROOT}/${ANDROID_TOOLCHAIN_MACHINE_NAME}")
    endif()
  endif()

  set(CMAKE_C_COMPILER   "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-clang"   CACHE PATH "C")
  set(CMAKE_CXX_COMPILER "${ANDROID_TOOLCHAIN_ROOT}/bin/${ANDROID_TOOLCHAIN_MACHINE_NAME}-clang++" CACHE PATH "C++")
  foreach(__lang C CXX)
    set(CMAKE_${__lang}_COMPILER_EXTERNAL_TOOLCHAIN "${ANDROID_TOOLCHAIN_ROOT}")
    set(CMAKE_${__lang}_SYSROOT_FLAG "-isysroot")
    set(CMAKE_${__lang}_COMPILER_TARGET "${ANDROID_TOOLCHAIN_MACHINE_NAME}")
  endforeach(__lang)
  set(CMAKE_SYSROOT "${ANDROID_TOOLCHAIN_ROOT}/sysroot")
endif()

set(ANDROID True)
add_definitions(-DANDROID)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
