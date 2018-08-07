#############################################################################
# $Id$
#############################################################################
#############################################################################
##
##  NCBI CMake wrapper: Test driver
##    Author: Andrei Gourianov, gouriano@ncbi
##


string(REPLACE " " ";" NCBITEST_ARGS    "${NCBITEST_ARGS}")
string(REPLACE " " ";" NCBITEST_ASSETS  "${NCBITEST_ASSETS}")

if (NOT "${NCBITEST_ASSETS}" STREQUAL "")
    list(REMOVE_DUPLICATES NCBITEST_ASSETS)
    foreach(_res IN LISTS NCBITEST_ASSETS)
        if (NOT EXISTS ${NCBITEST_SOURCEDIR}/${_res})
            message(SEND_ERROR "Test ${NCBITEST_NAME} ERROR: asset ${NCBITEST_SOURCEDIR}/${_res} not found")
            return()
        endif()
    endforeach()
endif()

string(RANDOM _subdir)
set(_subdir ${NCBITEST_NAME}_${_subdir})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_subdir})
set(_workdir ${CMAKE_CURRENT_BINARY_DIR}/${_subdir})
set(_output ${CMAKE_CURRENT_BINARY_DIR}/_test_out.${NCBITEST_NAME}.txt)
if(EXISTS ${_output})
    file(REMOVE ${_output})
endif()

foreach(_res IN LISTS NCBITEST_ASSETS)
    file(COPY ${NCBITEST_SOURCEDIR}/${_res} DESTINATION ${_workdir})
endforeach()

if(WIN32)
    string(REPLACE "/" "\\" NCBITEST_BINDIR  ${NCBITEST_BINDIR})
    set(ENV{PATH} "${NCBITEST_BINDIR}\\${NCBITEST_CONFIG};$ENV{PATH}")
else()
    set(ENV{PATH} ".:${NCBITEST_BINDIR}:$ENV{PATH}")
endif()

set(_result "1")
execute_process(
    COMMAND           ${NCBITEST_COMMAND} ${NCBITEST_ARGS}
    WORKING_DIRECTORY ${_workdir}
    TIMEOUT           ${NCBITEST_TIMEOUT}
    RESULT_VARIABLE   _result
    OUTPUT_FILE       ${_output}
    ERROR_FILE        ${_output}
)
file(REMOVE_RECURSE ${_workdir})

if (NOT ${_result} EQUAL "0")
    message(SEND_ERROR "Test ${NCBITEST_NAME} failed")
endif()
