macro(target_link_libraries_copied target)
  target_link_libraries(${target} ${ARGN})
  if (MSVC)
    foreach(__lib__ ${ARGN})
      if (NOT __lib__ MATCHES "^(PRIVATE|PUBLIC|INTERFACE)$")
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${__lib__}>" "$<TARGET_FILE_DIR:${target}>")
        set_target_properties(${target} PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${__lib__}>") # FIXME: Generator Expressions not supported here
      endif()
    endforeach()
  endif()
endmacro()
