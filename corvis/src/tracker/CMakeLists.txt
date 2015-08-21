add_executable(rc_replay rc_replay.cpp main.cpp)
target_include_directories(rc_replay PRIVATE
  .
  ../filter # FIXME: move rc_intel_interface.h here instead
)
set_target_properties(rc_replay PROPERTIES CXX_STANDARD 14)
target_link_libraries(rc_replay tracker)
