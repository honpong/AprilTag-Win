project(util CXX)

add_library(gtest_main           STATIC ../../ThirdParty/googletest/gtest/gtest_main.cc)
add_library(gtest                STATIC ../../ThirdParty/googletest/gtest/gtest-all.cc)
target_include_directories(gtest PUBLIC ../../ThirdParty/googletest)

add_library(util STATIC util.cpp)
target_link_libraries(util gtest trackeri)
target_include_directories(util PUBLIC .)
set_property(TARGET util PROPERTY CXX_STANDARD 11)

add_subdirectory(tracker)
if(ENABLE_STEREO)
  add_subdirectory(stereo)
endif()
