# ------------------------------------------------------------
# C langage samplefiles
# 
#  aotmatically build
#  ~/samples/C_Samples directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set library and comple options 
#set(SSM_LIBS ${SSM_LIBS} ssm)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/samples/C_Samples)


# build SensorAHandler
add_executable(SensorAHandler SensorAHandler.c)
target_link_libraries(SensorAHandler ${SSM_LIBS} m)


# build SensorAContinualRead
add_executable(SensorAContinualRead SensorAContinualRead.c)
target_link_libraries(SensorAContinualRead ${SSM_LIBS} m)


# build SensorAIntervalRead
add_executable(SensorAIntervalRead SensorAIntervalRead.c)
target_link_libraries(SensorAIntervalRead ${SSM_LIBS} m)


# build SensorAPastRead
add_executable(SensorAPastRead SensorAPastRead.c)
target_link_libraries(SensorAPastRead ${SSM_LIBS} m)


# build SensorBHandler
add_executable(SensorBHandler SensorBHandler.c)
target_link_libraries(SensorBHandler ${SSM_LIBS} m)


# build SensorABSync
add_executable(SensorABSync SensorABSync.c)
target_link_libraries(SensorABSync ${SSM_LIBS} m)
