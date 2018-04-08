project(util CXX)

add_library(gtest_main                STATIC  ../ThirdParty/googletest/src/gtest_main.cc)
target_include_directories(gtest_main PUBLIC  ../ThirdParty/googletest/include)
add_library(gtest                     STATIC  ../ThirdParty/googletest/src/gtest-all.cc)
target_include_directories(gtest      PRIVATE ../ThirdParty/googletest
                                      PUBLIC  ../ThirdParty/googletest/include)

add_library(util STATIC util.cpp)
target_link_libraries(util gtest trackeri)
target_include_directories(util PUBLIC .)
set_property(TARGET util PROPERTY CXX_STANDARD 11)

add_subdirectory(tracker)
if(ENABLE_STEREO)
  add_subdirectory(stereo)
endif()
