project(spec_plus CXX)

add_executable(spec_plus stereo.cpp)
set_property(TARGET spec_plus PROPERTY CXX_STANDARD 14)
target_link_libraries(spec_plus util gtest gtest_main stereo)
add_test(NAME spec_plus COMMAND spec_plus)
