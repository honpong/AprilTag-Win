add_executable(check_radius
  check_radius.cpp
)
set_property(TARGET check_radius PROPERTY CXX_STANDARD 14)
target_link_libraries(check_radius trackeri)
target_include_directories(check_radius PRIVATE ../measure)
