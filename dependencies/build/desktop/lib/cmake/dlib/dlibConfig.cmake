# ===================================================================================
#  The dlib CMake configuration file
#
#             ** File generated automatically, do not modify **
#
#  Usage from an external project:
#    In your CMakeLists.txt, add these lines:
#
#    FIND_PACKAGE(dlib REQUIRED)
#    TARGET_LINK_LIBRARIES(MY_TARGET_NAME ${dlib_LIBRARIES})
#
#    This file will define the following variables:
#      - dlib_LIBRARIES                : The list of all imported targets for dlib modules.
#      - dlib_INCLUDE_DIRS             : The dlib include directories.
#      - dlib_VERSION                  : The version of this dlib build.
#      - dlib_VERSION_MAJOR            : Major version part of this dlib revision.
#      - dlib_VERSION_MINOR            : Minor version part of this dlib revision.
#
# ===================================================================================



 
# Our library dependencies (contains definitions for IMPORTED targets)
if(NOT TARGET dlib-shared AND NOT dlib_BINARY_DIR)
   # Compute paths
   get_filename_component(dlib_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
   include("${dlib_CMAKE_DIR}/dlib.cmake")
endif()

set(dlib_LIBRARIES dlib::dlib)
set(dlib_LIBS      dlib::dlib)
set(dlib_INCLUDE_DIRS "/home/circleci/project/libheaders/desktop/include" "")

mark_as_advanced(dlib_LIBRARIES)
mark_as_advanced(dlib_LIBS)
mark_as_advanced(dlib_INCLUDE_DIRS)

# Mark these variables above as deprecated.
function(__deprecated_var var access)
   if(access STREQUAL "READ_ACCESS")
      message(WARNING "The variable '${var}' is deprecated!  Instead, simply use target_link_libraries(your_app dlib::dlib).  See http://dlib.net/examples/CMakeLists.txt.html for an example.")
   endif()
endfunction()
variable_watch(dlib_LIBRARIES __deprecated_var)
variable_watch(dlib_LIBS __deprecated_var)
variable_watch(dlib_INCLUDE_DIRS __deprecated_var)



