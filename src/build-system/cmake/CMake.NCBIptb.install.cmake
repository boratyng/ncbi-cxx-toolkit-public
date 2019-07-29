#############################################################################
# $Id$
#############################################################################
#############################################################################
##
##  NCBI CMake wrapper extension
##  In NCBI CMake wrapper, adds installation commands
##    Author: Andrei Gourianov, gouriano@ncbi
##


##############################################################################
function(NCBI_internal_install_target _variable _access)
    if(NOT "${_access}" STREQUAL "MODIFIED_ACCESS")
        return()
    endif()

    if (${NCBI_${NCBI_PROJECT}_TYPE} STREQUAL "STATIC")
        set(_haspdb NO)
        file(RELATIVE_PATH _dest "${NCBI_TREE_ROOT}" "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    elseif (${NCBI_${NCBI_PROJECT}_TYPE} STREQUAL "SHARED")
        set(_haspdb YES)
        if (WIN32)
            file(RELATIVE_PATH _dest    "${NCBI_TREE_ROOT}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
            file(RELATIVE_PATH _dest_ar "${NCBI_TREE_ROOT}" "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
        else()
            file(RELATIVE_PATH _dest "${NCBI_TREE_ROOT}" "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
        endif()
    elseif (${NCBI_${NCBI_PROJECT}_TYPE} STREQUAL "CONSOLEAPP" OR ${NCBI_${NCBI_PROJECT}_TYPE} STREQUAL "GUIAPP")
        set(_haspdb YES)
        file(RELATIVE_PATH _dest "${NCBI_TREE_ROOT}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        if (NOT "${NCBI_PTBCFG_INSTALL_TAGS}" STREQUAL "")
            set(_alltags ${NCBI__PROJTAG} ${NCBI_${NCBI_PROJECT}_PROJTAG})
            set(_res FALSE)
            set(_hasp FALSE)
            foreach(_tag IN LISTS NCBI_PTBCFG_INSTALL_TAGS)
                NCBI_util_parse_sign( ${_tag} _value _negate)
                if(_negate)
                    if( ${_value} IN_LIST _alltags)
                        if(NCBI_VERBOSE_ALLPROJECTS OR NCBI_VERBOSE_PROJECT_${NCBI_PROJECT})
                            message("${NCBI_PROJECT} will not be installed because of tag ${_value}")
                        endif()
                        return()
                    endif()
                else()
                    set(_hasp TRUE)
                    if( ${_value} IN_LIST _alltags OR ${_value} STREQUAL "*")
                        set(_res TRUE)
                    endif()
                endif()
            endforeach()
            if(_hasp AND NOT _res)
                if(NCBI_VERBOSE_ALLPROJECTS OR NCBI_VERBOSE_PROJECT_${NCBI_PROJECT})
                    message("${NCBI_PROJECT} will not be installed because of tags ${_alltags}")
                endif()
                return()
            endif()
        endif()
    else()
        return()
    endif()
    if ("${_dest}" STREQUAL "")
        return()
    endif()

# not sure about this part
    file(RELATIVE_PATH _rel "${NCBI_SRC_ROOT}" "${NCBI_CURRENT_SOURCE_DIR}")
    string(REPLACE "/" ";" _rel ${_rel})
    list(GET _rel 0 _dir)
    get_property(_all_subdirs GLOBAL PROPERTY NCBI_PTBPROP_ROOT_SUBDIR)
    list(APPEND _all_subdirs ${_dir})
    if (DEFINED NCBI_${NCBI_PROJECT}_PARTS)
        foreach(_rel IN LISTS NCBI_${NCBI_PROJECT}_PARTS)
            string(REPLACE "/" ";" _rel ${_rel})
            list(GET _rel 0 _dir)
            list(APPEND _all_subdirs ${_dir})
        endforeach()
    endif()
    list(REMOVE_DUPLICATES _all_subdirs)
    set_property(GLOBAL PROPERTY NCBI_PTBPROP_ROOT_SUBDIR ${_all_subdirs})

    if (WIN32 OR XCODE)
        foreach(_cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
            if (DEFINED _dest_ar)
                install(
                    TARGETS ${NCBI_PROJECT}
                    EXPORT ${NCBI_PTBCFG_INSTALL_EXPORT}${_cfg}
                    RUNTIME DESTINATION ${_dest}/${_cfg}
                    CONFIGURATIONS ${_cfg}
                    ARCHIVE DESTINATION ${_dest_ar}/${_cfg}
                    CONFIGURATIONS ${_cfg}
                )
            else()
                install(
                    TARGETS ${NCBI_PROJECT}
                    EXPORT ${NCBI_PTBCFG_INSTALL_EXPORT}${_cfg}
                    DESTINATION ${_dest}/${_cfg}
                    CONFIGURATIONS ${_cfg}
                )
            endif()
            if (WIN32 AND _haspdb)
                install(FILES $<TARGET_PDB_FILE:${NCBI_PROJECT}>
                        DESTINATION ${_dest}/${_cfg} OPTIONAL
                        CONFIGURATIONS ${_cfg})
            endif()
        endforeach()
    else()
        install(
            TARGETS ${NCBI_PROJECT}
            EXPORT ${NCBI_PTBCFG_INSTALL_EXPORT}
            DESTINATION ${_dest}
        )
    endif()
endfunction()

##############################################################################
function(NCBI_internal_export_hostinfo _file)
    if(EXISTS ${_file})
        file(REMOVE ${_file})
    endif()
    get_property(_allprojects     GLOBAL PROPERTY NCBI_PTBPROP_ALL_PROJECTS)
    if (NOT "${_allprojects}" STREQUAL "")
        set(_hostinfo)
        foreach(_prj IN LISTS _allprojects)
            get_property(_prjhost GLOBAL PROPERTY NCBI_PTBPROP_HOST_${_prj})
            if (NOT "${_prjhost}" STREQUAL "")
                list(APPEND _hostinfo "${_prj} ${_prjhost}\n")
            endif()
        endforeach()
        if (NOT "${_hostinfo}" STREQUAL "")
            file(WRITE ${_file} ${_hostinfo})
        endif()
    endif()
endfunction()

##############################################################################
function(NCBI_internal_install_root _variable _access)
    if(NOT "${_access}" STREQUAL "MODIFIED_ACCESS")
        return()
    endif()

    file(RELATIVE_PATH _dest "${NCBI_TREE_ROOT}" "${NCBI_BUILD_ROOT}")
    set(_hostinfo ${NCBI_BUILD_ROOT}/${NCBI_DIRNAME_BUILD}/${CMAKE_PROJECT_NAME}.hostinfo)
    NCBI_internal_export_hostinfo(${_hostinfo})
    if (EXISTS ${_hostinfo})
        install( FILES ${_hostinfo} DESTINATION ${_dest}/${NCBI_DIRNAME_EXPORT} RENAME ${NCBI_PTBCFG_INSTALL_EXPORT}.hostinfo)
    endif()

    if (WIN32 OR XCODE)
        foreach(_cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
            install(EXPORT ${NCBI_PTBCFG_INSTALL_EXPORT}${_cfg}
                CONFIGURATIONS ${_cfg}
                DESTINATION ${_dest}/${NCBI_DIRNAME_EXPORT}
                FILE ${NCBI_PTBCFG_INSTALL_EXPORT}.cmake
            )
        endforeach()
    else()
        install(EXPORT ${NCBI_PTBCFG_INSTALL_EXPORT}
            DESTINATION ${_dest}/${NCBI_DIRNAME_EXPORT}
            FILE ${NCBI_PTBCFG_INSTALL_EXPORT}.cmake
        )
    endif()

# install headers
    get_property(_all_subdirs GLOBAL PROPERTY NCBI_PTBPROP_ROOT_SUBDIR)
    list(APPEND _all_subdirs ${NCBI_DIRNAME_COMMON_INCLUDE})
    foreach(_dir IN LISTS _all_subdirs)
        if (EXISTS ${NCBI_INC_ROOT}/${_dir})
            install( DIRECTORY ${NCBI_INC_ROOT}/${_dir} DESTINATION ${NCBI_DIRNAME_INCLUDE}
                REGEX "/[.].*$" EXCLUDE)
        endif()
    endforeach()
    file(GLOB _files LIST_DIRECTORIES false "${NCBI_INC_ROOT}/*")
    install( FILES ${_files} DESTINATION ${NCBI_DIRNAME_INCLUDE})

# install sources?
    # TODO

    file(GLOB _files LIST_DIRECTORIES false "${NCBI_TREE_BUILDCFG}/*")
    install( FILES ${_files} DESTINATION ${NCBI_DIRNAME_BUILDCFG})
    install( DIRECTORY ${NCBI_TREE_CMAKECFG} DESTINATION ${NCBI_DIRNAME_BUILDCFG}
            USE_SOURCE_PERMISSIONS REGEX "/[.].*$" EXCLUDE)

    install( DIRECTORY ${NCBI_TREE_ROOT}/${NCBI_DIRNAME_COMMON_SCRIPTS} DESTINATION ${NCBI_DIRNAME_SCRIPTS}
            USE_SOURCE_PERMISSIONS REGEX "/[.].*$" EXCLUDE)

    file(RELATIVE_PATH _dest "${NCBI_TREE_ROOT}" "${NCBI_BUILD_ROOT}")
    install( DIRECTORY ${NCBI_CFGINC_ROOT} DESTINATION "${_dest}"
            REGEX "/[.].*$" EXCLUDE)
endfunction()

#############################################################################
NCBI_register_hook(TARGETDONE  NCBI_internal_install_target)
NCBI_register_hook(CFGDONE     NCBI_internal_install_root)
