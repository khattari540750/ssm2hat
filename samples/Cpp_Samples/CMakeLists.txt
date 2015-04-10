# ------------------------------------------------------------
# C++ langage samplefiles
# 
#  aotmatically build
#  ~/samples/Cpp_Samples directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/src)


# set library and comple options 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/samples/Cpp_Samples)


# build ssm_write_sample
add_executable(ssm_write_sample ssmWriteSample.cpp)
target_link_libraries(ssm_write_sample ssm m)


# build ssm_read_sample
add_executable(ssm_read_sample ssmReadSample.cpp)
target_link_libraries(ssm_read_sample ssm m)