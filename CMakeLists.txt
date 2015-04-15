# ------------------------------------------------------------
#  Root CMake file for ssm
#
#    From the off-tree build directory, invoke:
#      $ cmake <PATH_TO_SSM_ROOT>
#
# ------------------------------------------------------------


# Project setting
cmake_minimum_required(VERSION 2.4 FATAL_ERROR)
project(ssm2hat)


# Project version number
set(VERSION_NUMBER_MAJOR 0)
set(VERSION_NUMBER_MINOR 0)
set(VERSION_NUMBER_MICRO 0)


# configure a header file to pass some of the CMake settings 
# to the source code
configure_file(
  "${PROJECT_SOURCE_DIR}/include/config.h.in"
  "${PROJECT_SOURCE_DIR}/include/config.h"
)


# add the sub directories
add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(samples)
add_subdirectory(utilities)
