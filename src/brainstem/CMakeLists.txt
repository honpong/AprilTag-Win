project(hub CXX)

add_subdirectory(../../ThirdParty/brainstem brainstem)

if (TARGET BrainStem2)
  add_executable(${PROJECT_NAME} main.cpp)
  target_link_libraries(${PROJECT_NAME} BrainStem2)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
endif()

