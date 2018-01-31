add_library(loop_gt_generator STATIC
    batch_gt_generator.cpp
    batch_gt_generator.h
    gt_generator.cpp
    gt_generator.h
    loop_tester.cpp
    loop_tester.h
    image_reader.cpp
    image_reader.h
    symmetric_matrix.h
)
set_property(TARGET loop_gt_generator PROPERTY CXX_STANDARD 14)
target_link_libraries(loop_gt_generator trackeri)
target_include_directories(loop_gt_generator INTERFACE ./)

add_executable(generate_loop_gt
  generate_loop_gt.cpp
  ../measure/for_each_file.cpp
)
set_property(TARGET generate_loop_gt PROPERTY CXX_STANDARD 14)
target_include_directories(generate_loop_gt PRIVATE ../measure)
target_link_libraries(generate_loop_gt loop_gt_generator)

find_package(TBB QUIET)
if (TARGET tbb)
    set_source_files_properties(batch_gt_generator.cpp
        PROPERTIES COMPILE_DEFINITIONS HAVE_TBB=1)
    target_link_libraries(loop_gt_generator tbb)
endif ()
