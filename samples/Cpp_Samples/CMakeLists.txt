# ------------------------------------------------------------
#  samples CMake file for ssm
#
#    auotmatically build
#
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set library and comple options 
set(SSM_LIBS ${SSM_LIBS} ssm)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/samples/Cpp_Samples)

## ssmWriteSample
add_executable(ssmWriteSample ssmWriteSample.cpp)
target_link_libraries(ssmWriteSample ${SSM_LIBS} m)

## ssmReadSample
add_executable(ssmReadSample ssmReadSample.cpp)
target_link_libraries(ssmReadSample ${SSM_LIBS} m)
