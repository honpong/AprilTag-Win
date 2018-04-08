cmake_minimum_required(VERSION 3.2.2)

project(rc_replay CXX)

include(TargetLinkLibrariesCopied OPTIONAL RESULT_VARIABLE TARGET_LINK_LIBRARIES_COPIED)
if (TARGET_LINK_LIBRARIES_COPIED STREQUAL "NOTFOUND")
  if (MSVC)
    message(WARNING "Unable to find TargetLinkLibrariesCopied.cmake; add its directory to CMAKE_MODULE_PATH copy the .dlls by hand." )
  endif()
  function(target_link_libraries_copied)
    target_link_libraries(${ARGV})
  endfunction()
endif()

if (NOT TARGET tracker)
  find_package(tracker)
endif()

add_executable(rc_replay rc_replay.h rc_replay.cpp main.cpp)
set_target_properties(rc_replay PROPERTIES CXX_STANDARD 11)
target_link_libraries_copied(rc_replay tracker)
install(TARGETS rc_replay RUNTIME DESTINATION bin)
