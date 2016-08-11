cmake_minimum_required(VERSION 3.2.2 FATAL_ERROR)

project(qhull)

if (NOT TARGET qhull)
  add_subdirectory(qhull)
  foreach(qvar qhull_TARGETS qhull_STATIC qhull_SHARED qhull_SHAREDP)
    get_directory_property(${qvar} DIRECTORY qhull DEFINITION ${qvar})
  endforeach(qvar)
  set_target_properties(${qhull_TARGETS} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
  set_target_properties(${qhull_SHARED} ${qhull_SHAREDP} PROPERTIES MACOSX_RPATH ON) # Avoid pointless warning
  target_include_directories(${qhull_STATIC} PUBLIC qhull/src/libqhull)
endif()
