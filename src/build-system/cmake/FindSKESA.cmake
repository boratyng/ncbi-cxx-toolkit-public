#
#
# Find the wgMLST library, part of SKESA distribution.

# This will include
# WGMLST_FOUND
# WGMLST_INCLUDE_DIRS
# WGMLST_LIBRARIES
# WGMLST_LIBPATH

find_package(LIBWGMLST REQUIRED
                PATHS /panfs/pan1/gpipe/ThirdParty/skesa-1.1/
                NO_DEFAULT_PATH
)

if (LIBWGMLST_FOUND)
    set(WGMLST_FOUND TRUE)
    set(WGMLST_INCLUDE_DIRS ${LIBWGMLST_INCLUDE_DIRS})
    set(WGMLST_LIBRARIES ${LIBWGMLST_LIBRARIES})
    set(WGMLST_VERSION_STRING ${LIBWGMLST_VERSION})

    set(WGMLST_LIBPATH -L${LIBWGMLST_LIBRARY_DIRS} -Wl,-rpath,${LIBWGMLST_LIBRARY_DIRS})
    message(STATUS "Found wgMLST: ${WGMLST_LIBRARIES} (found version \"${WGMLST_VERSION_STRING}\")")
else()
    set(WGMLST_FOUND FALSE)
endif()
