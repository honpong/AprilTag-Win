add_executable(generate_loop_gt
  generate_loop_gt.cpp
  image_reader.cpp
  image_reader.h
  symmetric_matrix.h
  ../measure/for_each_file.cpp
)
set_property(TARGET generate_loop_gt PROPERTY CXX_STANDARD 14)
target_link_libraries(generate_loop_gt trackeri)
target_include_directories(generate_loop_gt PRIVATE ../measure)

find_package(TBB QUIET)
if (TARGET tbb)
    set_source_files_properties(generate_loop_gt.cpp
        PROPERTIES COMPILE_DEFINITIONS HAVE_TBB=1)
    target_link_libraries(generate_loop_gt tbb)
endif ()
