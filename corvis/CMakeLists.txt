cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

set(PACKAGE_NAME "corvis")

project(${PACKAGE_NAME} C CXX)

list(APPEND CMAKE_PREFIX_PATH ../cmake)

if (CMAKE_CXX_COMPILER_ID STREQUAL Clang AND EXISTS /usr/include/c++/v1)
  include_directories(BEFORE SYSTEM /usr/include/c++/v1)
  link_libraries(c++)
endif()

find_package(alloy QUIET)
if (TARGET alloy)
    add_compile_options(-march=atom -mfpmath=sse)
elseif(WIN32 OR ANDROID)
else()
    add_compile_options(-march=native -mfpmath=sse)
endif()

set(ENABLE_STEREO true)
set(ENABLE_MGL true)
set(ENABLE_VISGL true)
set(ENABLE_QR true)
if(WIN32)
    set(ENABLE_STEREO false)
    set(ENABLE_MGL false)
    set(ENABLE_VISGL true)
    set(ENABLE_QR false)
elseif(ANDROID)
    set(ENABLE_STEREO false)
    set(ENABLE_MGL false)
    set(ENABLE_VISGL false)
    set(ENABLE_QR false)
endif()

# A few hints for the BOOST_ROOT to help find_package(Boost)
execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
if (BREW_PREFIX)
  file(GLOB _PREFIX ${BREW_PREFIX} /opt/local)
  set(BOOST_ROOT ${_PREFIX})
endif (BREW_PREFIX)

add_subdirectory(src)

if(ENABLE_VISGL)
add_executable(measure
  src/tools/measure.cpp
  src/tools/benchmark.cpp
  src/tools/for_each_file.cpp
)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)
install(TARGETS measure RUNTIME DESTINATION bin COMPONENT alloy)

target_link_libraries(measure tracker)
target_link_libraries(measure vis)
endif()
if(ENABLE_MGL)
    target_link_libraries(measure mgl)
endif()

add_executable(rc_replay src/tools/rc_replay.cpp src/tracker/rc_replay.cpp)
set_target_properties(rc_replay PROPERTIES CXX_STANDARD 14)
target_link_libraries(rc_replay tracker)
install(TARGETS rc_replay RUNTIME DESTINATION bin COMPONENT example-binaries)
install(FILES
  src/tools/rc_replay.cpp
  RENAME main.cpp
  DESTINATION src/rc_replay
  COMPONENT example-sources
)
install(FILES
  src/tracker/rc_replay.h
  src/tracker/rc_replay.cpp
  DESTINATION src/rc_replay
  COMPONENT example-sources
)

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

if(YES)
    enable_testing()
    set(spec_sources
      ../gtest/gtest_main.cc ../gtest/gtest-all.cc
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
    target_include_directories(spec PRIVATE test ..)
    target_include_directories(spec PRIVATE ../ThirdParty/spdlog/include/)
    target_link_libraries(spec filter)
    if(ENABLE_QR)
       target_link_libraries(spec qr)
    endif()
    add_test(NAME spec COMMAND spec)

    add_executable(spec_intel
      ../gtest/gtest_main.cc ../gtest/gtest-all.cc
      src/filter/rc_tracker.cpp
      test/filter/rc_tracker_tests.cpp
    )
    set_property(TARGET spec_intel PROPERTY CXX_STANDARD 14)
    target_include_directories(spec_intel PRIVATE test ..)
    target_link_libraries(spec_intel filter)
    add_test(NAME spec_intel COMMAND spec_intel)

    if(BUILD_STEREO)
        add_executable(spec_plus
          ../gtest/gtest_main.cc ../gtest/gtest-all.cc
          test/test.cpp test/util.cpp
          test/stereo/stereo.cpp
        )
        set_property(TARGET spec_plus PROPERTY CXX_STANDARD 14)
        target_include_directories(spec_plus PRIVATE test ..)
        target_link_libraries(spec_plus filter stereo)
        add_test(NAME spec_plus COMMAND spec_plus)
    endif()
endif()
