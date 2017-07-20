add_executable(compare_poses
  compare_poses.cpp
)
set_property(TARGET compare_poses PROPERTY CXX_STANDARD 14)
target_link_libraries(compare_poses trackeri)
target_include_directories(compare_poses PRIVATE ../measure)
