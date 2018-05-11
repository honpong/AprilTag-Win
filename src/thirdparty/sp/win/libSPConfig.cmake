get_filename_component(libSP_CONFIG_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY CACHE)

set(libSP_VERSION "20180511" CACHE STRING "Choose either trunk / gold2 / 20171211 / 20180511")

if(WIN32)

    set(libSP_DIR "${libSP_CONFIG_PATH}/${libSP_VERSION}")

    if(EXISTS ${libSP_DIR})

        set(libSP_FOUND TRUE CACHE BOOL "" FORCE)

        if(${libSP_VERSION} MATCHES "trunk" OR ${libSP_VERSION} MATCHES "gold2")
            set(libSP_LIB "SP_Core")
        else()
            set(libSP_LIB "DenseReconstruction")
        endif()

        add_library(libSP SHARED IMPORTED)
        set_target_properties(libSP PROPERTIES 
            INCLUDE_DIRECTORIES           ${libSP_DIR}/include
            INTERFACE_INCLUDE_DIRECTORIES ${libSP_DIR}/include
            IMPORTED_LOCATION             ${libSP_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}/${libSP_LIB}.dll
            IMPORTED_IMPLIB               ${libSP_DIR}/lib/${CMAKE_VS_PLATFORM_NAME}/${libSP_LIB}.lib
        ) 
        
        MESSAGE(STATUS "SP Path ${libSP_DIR}/include")

    else()
        set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
        MESSAGE(STATUS "SP Path not found")
    endif()
    
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	MESSAGE("Darwin SP not supported yet")
	set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(libSP_FOUND FALSE CACHE BOOL "" FORCE)
	MESSAGE("Linux SP not supported yet")
endif()

