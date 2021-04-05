
#
# Config to dump diagnostic information at the completion of configuration
#

get_directory_property(Defs COMPILE_DEFINITIONS)
foreach (d ${Defs} )
    set(DefsStr "${DefsStr} -D${d}")
endforeach()

if ( NOT "${NCBI_MODULES_FOUND}" STREQUAL "")
    list(REMOVE_DUPLICATES NCBI_MODULES_FOUND)
endif()
foreach (mod ${NCBI_MODULES_FOUND})
    set(MOD_STR "${MOD_STR} ${mod}")
endforeach()

#STRING(SUBSTRING "${EXTERNAL_LIBRARIES_COMMENT}" 1 -1 EXTERNAL_LIBRARIES_COMMENT)

function(ShowMainBoilerplate)
    message("")
    if (WIN32 OR XCODE)
        message("NCBI_SIGNATURE:        ${NCBI_SIGNATURE_CFG}")
    else()
        message("NCBI_SIGNATURE:        ${NCBI_SIGNATURE}")
    endif()
    if(DEFINED NCBITEST_SIGNATURE)
        message("NCBITEST_SIGNATURE:    ${NCBITEST_SIGNATURE}")
    endif()
    if($ENV{NCBI_AUTOMATED_BUILD})
        message("NCBI_AUTOMATED_BUILD:  $ENV{NCBI_AUTOMATED_BUILD}")
    endif()
    if($ENV{NCBI_CHECK_DB_LOAD})
        message("NCBI_CHECK_DB_LOAD:    $ENV{NCBI_CHECK_DB_LOAD}")
    endif()
    if($ENV{NCBIPTB_INSTALL_CHECK})
        message("NCBIPTB_INSTALL_CHECK: $ENV{NCBIPTB_INSTALL_CHECK}")
    endif()
    if($ENV{NCBIPTB_INSTALL_SRC})
        message("NCBIPTB_INSTALL_SRC:   $ENV{NCBIPTB_INSTALL_SRC}")
    endif()
message("------------------------------------------------------------------------------")
    message("CMake exe:      ${CMAKE_COMMAND}")
    message("CMake version:  ${CMAKE_VERSION}")
    message("Build Target:   ${CMAKE_BUILD_TYPE}")
    message("Shared Libs:    ${BUILD_SHARED_LIBS}")
    message("Top Source Dir: ${top_src_dir}")
    message("Build Root:     ${build_root}")
    message("Executable Dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    message("Archive Dir:    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    message("Library Dir:    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message("C Compiler:     ${CMAKE_C_COMPILER}")
    message("C++ Compiler:   ${CMAKE_CXX_COMPILER}")
    if (CMAKE_USE_DISTCC AND DISTCC_EXECUTABLE)
        message("    distcc:     ${DISTCC_EXECUTABLE}")
    endif()
    if (CMAKE_USE_CCACHE AND CCACHE_EXECUTABLE)
        message("    ccache:     ${CCACHE_EXECUTABLE}")
    endif()
    message("CFLAGS:        ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}")
    message("CXXFLAGS:      ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
    message("LINKER_FLAGS:  ${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE}}")
    message("Compile Flags: ${DefsStr}")
    message("DataTool Ver:   ${_datatool_version}")
    message("DataTool Path:  ${NCBI_DATATOOL}")
    message("")
    message("Components:  ${NCBI_ALL_COMPONENTS}")
    message("Other features:  ${NCBI_ALL_REQUIRES} ${NCBI_PTBCFG_PROJECT_FEATURES}")
    message("Deprecated components:  ${NCBI_ALL_LEGACY}")

    message("------------------------------------------------------------------------------")
    message("")
endfunction()

ShowMainBoilerplate()