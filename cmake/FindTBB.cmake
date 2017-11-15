find_library(TBB_LIBRARY tbb)
find_path(TBB_INCLUDE tbb/tbb.h)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tbb DEFAULT_MSG TBB_INCLUDE TBB_LIBRARY)
if(TBB_FOUND AND NOT TARGET tbb)
  add_library(tbb INTERFACE)
  target_include_directories(tbb INTERFACE ${TBB_INCLUDE})
  target_link_libraries(tbb INTERFACE ${TBB_LIBRARY})
endif()
