get_filename_component(libSP_CONFIG_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY CACHE)

set(SP_TYPE "20171204" CACHE STRING "Choose either trunk / gold2 / 20171204")

if(WIN32)
if(${SP_TYPE} MATCHES "trunk" OR ${SP_TYPE} MATCHES "gold2")

	set(libSP_ROOT "${libSP_CONFIG_PATH}/${SP_TYPE}")
	set(libSP_INCLUDE_DIR "${libSP_ROOT}/include")
	set(libSP_LIB_DIR "${libSP_ROOT}/lib/${CMAKE_VS_PLATFORM_NAME}")
	set(libSP_BIN_DIR "${libSP_ROOT}/bin/${CMAKE_VS_PLATFORM_NAME}")
	set(libSP_LIB "SP_Core")
	set(libSP_FOUND TRUE CACHE BOOL "" FORCE)

	MESSAGE("SP Path ${libSP_INCLUDE_DIR}")

elseif(${SP_TYPE} MATCHES "20171204")

	set(libSP_ROOT "${libSP_CONFIG_PATH}/${SP_TYPE}")
	set(libSP_INCLUDE_DIR "${libSP_ROOT}/include")
	set(libSP_LIB_DIR "${libSP_ROOT}/lib/${CMAKE_VS_PLATFORM_NAME}")
	set(libSP_BIN_DIR "${libSP_ROOT}/bin/${CMAKE_VS_PLATFORM_NAME}")
	set(libSP_LIB "DenseReconstruction")
	set(libSP_FOUND TRUE CACHE BOOL "" FORCE)

	MESSAGE("SP Path ${libSP_INCLUDE_DIR}")

else()
	set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	MESSAGE("Darwin SP not supported yet")
	set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
	MESSAGE("Linux SP not supported yet")
endif()

MESSAGE( STATUS "libSP library name: " ${libSP_LIB} )
