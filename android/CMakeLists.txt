cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(Tracker)

add_subdirectory(../corvis "${CMAKE_CURRENT_BINARY_DIR}/corvis")

set_target_properties(tracker PROPERTIES DEBUG_POSTFIX _debug)
set_target_properties(tracker PROPERTIES RELWITHDEBINFO_POSTFIX _relwithdebinfo)

set_target_properties(spec spec_intel PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
