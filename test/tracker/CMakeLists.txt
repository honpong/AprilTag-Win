project(spec C CXX)

add_executable(spec
  vec4.cpp
  rotation_vector.cpp
  quaternion.cpp
  transformation_cov.cpp
  estimate_horn.cpp
  estimate_epnp.cpp
  estimate_five_point.cpp
  matrix.cpp
  remapper.cpp
  utils.cpp
  state_vision.cpp
  calibration_json.cpp
  tpose.cpp
  mapper.cpp
  fast_tracker.cpp
  sensor_fusion_queue.cpp
  $<$<BOOL:${ENABLE_QR}>:
     homography.cpp
  >
  bstream.cpp
)
target_link_libraries(spec util gtest gtest_main trackeri)

add_test(NAME spec COMMAND spec --gtest_output=xml:spec.xml)

add_executable(spec_integration
    stages.cpp
)
target_link_libraries(spec_integration util gtest gtest_main trackeri tracker)
set_property(TARGET spec_integration PROPERTY CXX_STANDARD 14)
add_test(NAME spec_integration COMMAND spec_integration --gtest_output=xml:spec_integration.xml WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(spec_intel
  rc_tracker_tests.cpp
  prediction.cpp
)
target_link_libraries(spec_intel util gtest gtest_main tracker trackeri)
set_property(TARGET spec_intel PROPERTY CXX_STANDARD 14)
add_test(NAME spec_intel COMMAND spec_intel --gtest_output=xml:spec_intel.xml)

set_property(TARGET spec spec_intel PROPERTY CXX_STANDARD 14)

add_executable(c-test c_test.c)
set_property(TARGET c-test PROPERTY C_STANDARD 99)
target_link_libraries(c-test tracker)
add_test(NAME c-test COMMAND c-test --gtest_output=xml:c-test.xml)
