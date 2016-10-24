cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(corvis C CXX)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake)

if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND EXISTS /usr/include/c++/v1)
  include_directories(BEFORE SYSTEM /usr/include/c++/v1)
  link_libraries(c++)
endif()

# SDL (Security Development Lifecycle) Requirements
set(CMAKE_POSITION_INDEPENDENT_CODE True)
if (MSVC)
  add_compile_options(/sdl)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel|GNU|Clang")
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES AppleClang)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z relro -z now -z noexecstack -pie")
  endif()
  add_definitions(-D_FORTIFY_SOURCE=2)
  add_compile_options(-ffunction-sections)
  add_compile_options(-fstack-protector-strong)
  add_compile_options(-Wformat -Wformat-security)
  add_compile_options(-fvisibility=hidden)
endif ()

macro(target_link_libraries_copied target)
  target_link_libraries(${target} ${ARGN})
  if (MSVC)
    foreach(__lib__ ${ARGN})
      if (NOT __lib__ MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${__lib__}>" "$<TARGET_FILE_DIR:${target}>")
        set_target_properties(${target} PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${__lib__}>") # FIXME: Generator Expressions not supported here
      endif()
    endforeach()
  endif()
endmacro()

find_package(alloy QUIET)
if (TARGET alloy)
  add_compile_options(-march=atom -mfpmath=sse)
elseif (WIN32)
elseif (ANDROID)
  # Match the options used by default in android-ndk-*/toolchains/*-clang3.6/setup.mk
  add_compile_options(-no-canonical-prefixes -funwind-tables -fno-omit-frame-pointer)
else (APPLE)
  add_compile_options(-march=native -mfpmath=sse)
endif ()

option(ENABLE_STEREO "Enable Stereo" OFF)
option(ENABLE_VISGL "Enable GL Visualizations" OFF)
option(ENABLE_MGL "Enable MathGL (used for plots in measure)" OFF)
option(ENABLE_QR "Enable QR Code localization" OFF)
option(ENABLE_SOURCES "Enable installation of source code examples" OFF)

# A few hints for the BOOST_ROOT to help find_package(Boost)
if (APPLE)
  execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(GLOB BREW_PREFIX ${BREW_PREFIX} /opt/local)
  list(APPEND CMAKE_PREFIX_PATH ${BREW_PREFIX})
endif (APPLE)

add_subdirectory(src)

if(ENABLE_VISGL)

set(GLFW_INSTALL OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
add_subdirectory(src/glfw)
if(NOT WIN32)
    target_compile_options(glfw PRIVATE $<$<CXX_COMPILER_ID:AppleClang>: -Wno-deprecated-declarations>)
endif()
target_include_directories(glfw INTERFACE src/glfw/include)

add_library(glad STATIC
  src/glfw/deps/glad.c
)
target_compile_options(glad PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4996>)
target_include_directories(glad
  PUBLIC src/glfw/deps
)

add_executable(measure
  src/measure/measure.cpp
  src/measure/benchmark.cpp
  src/measure/for_each_file.cpp

  src/measure/world_state.cpp
  src/measure/world_state_render.cpp
  src/measure/arcball.cpp
  src/measure/offscreen_render.cpp
  src/measure/gui.cpp
  src/measure/lodepng.cpp
  src/measure/gl_util.cpp
  src/measure/render.cpp
  src/measure/video_render.cpp
)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)
install(TARGETS measure RUNTIME DESTINATION bin COMPONENT alloy)
if(CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    target_compile_options(measure PRIVATE -DDEBUG=1)
endif()

target_link_libraries_copied(measure PUBLIC tracker)
if(ENABLE_MGL)
    target_link_libraries_copied(measure PRIVATE mgl)
endif()

target_link_libraries(measure PRIVATE glad glfw PUBLIC filter)
if(ENABLE_MGL)
    add_subdirectory(../ThirdParty/mathgl mathgl)
    target_compile_options(mgl PRIVATE $<$<BOOL:${MSVC}>:/wd4244 /wd4190 /wd4800>)
    target_link_libraries(measure PRIVATE mgl)
    target_compile_definitions(measure PRIVATE HAVE_MGL)
endif()
target_link_libraries(measure PRIVATE ${GLFW_LIBRARIES})
target_include_directories(measure PUBLIC measure)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)

endif() # VISGL

add_executable(rc_replay src/rc_replay/rc_replay.cpp src/rc_replay/main.cpp)
set_target_properties(rc_replay PROPERTIES CXX_STANDARD 14)
target_link_libraries_copied(rc_replay tracker)
install(TARGETS rc_replay RUNTIME DESTINATION bin COMPONENT example-binaries)
if(ENABLE_SOURCES)
install(FILES
  src/rc_replay/main.cpp
  src/rc_replay/rc_replay.h
  src/rc_replay/rc_replay.cpp
  DESTINATION src/rc_replay
  COMPONENT example-sources
)
endif()

add_executable(alloy_merge src/tools/alloy_merge.cpp)
set_property(TARGET alloy_merge PROPERTY CXX_STANDARD 14)
target_link_libraries(alloy_merge tracker)

find_package(alloy QUIET)
if (TARGET alloy)
    install(TARGETS alloy_merge RUNTIME DESTINATION bin COMPONENT alloy)

    add_executable(rc_calibrate
        src/tools/rc_calibrate.cpp
    )
    set_property(TARGET rc_calibrate PROPERTY CXX_STANDARD 14)
    target_link_libraries(rc_calibrate alloy tracker)
    install(TARGETS rc_calibrate RUNTIME DESTINATION bin COMPONENT alloy)

endif()

if(YES AND NOT ANDROID)
    enable_testing()
    set(spec_sources
      ../ThirdParty/googletest/gtest/gtest_main.cc
      ../ThirdParty/googletest/gtest/gtest-all.cc
      test/test.cpp test/util.cpp
      test/numerics/vec4.cpp test/numerics/rotation_vector.cpp test/numerics/quaternion.cpp
      test/numerics/transformation.cpp
      test/filter/utils.cpp
      test/filter/state_vision.cpp
      test/filter/calibration_json.cpp
      test/filter/tpose.cpp
      test/filter/dictionary.cpp
      test/filter/mapper.cpp
      test/filter/fast_tracker.cpp
      test/cor/sensor_fusion_queue.cpp
    )
    if(ENABLE_QR)
        set(spec_sources ${spec_sources} test/filter/homography.cpp)
    endif()
    add_executable(spec ${spec_sources})
    set_property(TARGET spec PROPERTY CXX_STANDARD 14)
    target_include_directories(spec PRIVATE test ../ThirdParty/googletest/)
    target_include_directories(spec PRIVATE ../ThirdParty/spdlog/include/)
    target_link_libraries(spec filter)
    if(ENABLE_QR)
       target_link_libraries(spec qr)
    endif()
    add_test(NAME spec COMMAND spec)

    add_executable(spec_intel
      ../ThirdParty/googletest/gtest/gtest_main.cc
      ../ThirdParty/googletest/gtest/gtest-all.cc
      src/filter/rc_tracker.cpp
      test/filter/rc_tracker_tests.cpp
    )
    set_property(TARGET spec_intel PROPERTY CXX_STANDARD 14)
    target_include_directories(spec_intel PRIVATE test ../ThirdParty/googletest/)
    target_link_libraries(spec_intel filter)
    add_test(NAME spec_intel COMMAND spec_intel)

    if(BUILD_STEREO)
        add_executable(spec_plus
          ../ThirdParty/googletest/gtest/gtest_main.cc
          ../ThirdParty/googletest/gtest/gtest-all.cc
          test/test.cpp test/util.cpp
          test/stereo/stereo.cpp
        )
        set_property(TARGET spec_plus PROPERTY CXX_STANDARD 14)
        target_include_directories(spec_plus PRIVATE test ../ThirdParty/googletest/)
        target_link_libraries(spec_plus filter stereo)
        add_test(NAME spec_plus COMMAND spec_plus)
    endif()
endif()

set(CPACK_PACKAGE_NAME "tracker")
set(CPACK_GENERATOR "ZIP")
if (NOT RC_VERSION)
    string(TIMESTAMP RC_VERSION "%Y-%m-%d")
endif()
set(CPACK_PACKAGE_VERSION ${RC_VERSION})
include(CPack)
