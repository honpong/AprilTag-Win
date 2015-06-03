cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

set(PACKAGE_NAME "corvis")

project(${PACKAGE_NAME} C CXX)

if(WIN32)
else()
    add_compile_options(-march=native -mfpmath=sse -O3)
endif()

set(ENABLE_STEREO true)
set(ENABLE_MGL true)
set(ENABLE_ZXING true)
if(WIN32)
    set(ENABLE_STEREO false)
    set(ENABLE_MGL false)
    set(ENABLE_ZXING false)
endif()

# A few hints for the BOOST_ROOT to help find_package(Boost)
execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
if (BREW_PREFIX)
  file(GLOB _PREFIX ${BREW_PREFIX} /opt/local)
  set(BOOST_ROOT ${_PREFIX})
endif (BREW_PREFIX)

add_subdirectory(src)

add_executable(measure
  src/tools/measure.cpp
  src/filter/replay.cpp
)
set_property(TARGET measure PROPERTY CXX_STANDARD 14)

target_link_libraries(measure
  filter
  numerics
  cor
  vis
  casablanca
)
if(ENABLE_MGL)
    target_link_libraries(measure mgl)
endif()

if(YES)
    add_executable(spec
      src/filter/rc_intel_interface.cpp
      ../gtest/gtest_main.cc ../gtest/gtest-all.cc
      test/test.cpp test/util.cpp
      test/numerics/vec4.cpp test/numerics/rotation_vector.cpp test/numerics/quaternion.cpp
      test/filter/homography.cpp test/filter/utils.cpp
      test/filter/device_parameters_tests.cpp
      test/filter/calibration_json_store_tests.cpp
      test/cor/sensor_fusion_queue.cpp
      test/filter/rc_intel_interface_tests.cpp
    )
    set_property(TARGET spec PROPERTY CXX_STANDARD 14)
    target_include_directories(spec PRIVATE test ..)
    target_link_libraries(spec
      cor
      numerics
      filter
      casablanca
    )

    if(BUILD_STEREO)
        add_executable(spec_plus
          ../gtest/gtest_main.cc ../gtest/gtest-all.cc
          test/test.cpp test/util.cpp
          test/stereo/stereo.cpp
        )
        set_property(TARGET spec_plus PROPERTY CXX_STANDARD 14)
        target_include_directories(spec_plus PRIVATE test ..)
        target_link_libraries(spec_plus
          cor
          numerics
          filter
          stereo
        )
    endif()
endif()
