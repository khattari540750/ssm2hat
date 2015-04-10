# ------------------------------------------------------------
# C langage samplefiles
# 
#  aotmatically build
#  ~/samples/C_Samples directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/src)


# set library and comple options 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/samples/C_Samples)


# build sensorA_handler
add_executable(sensorA_handler SensorAHandler.c)
target_link_libraries(sensorA_handler ssm m)


# build sensorA_continual_read
add_executable(sensorA_continual_read SensorAContinualRead.c)
target_link_libraries(sensorA_continual_read ssm m)


# build sensorA_interval_read
add_executable(sensorA_interval_read SensorAIntervalRead.c)
target_link_libraries(sensorA_interval_read ssm m)


# build sensorA_past_read
add_executable(sensorA_past_read SensorAPastRead.c)
target_link_libraries(sensorA_past_read ssm m)


# build sensorB_handler
add_executable(sensorB_handler SensorBHandler.c)
target_link_libraries(sensorB_handler ssm m)


# build sensorAB_sync
add_executable(sensorAB_sync SensorABSync.c)
target_link_libraries(sensorAB_sync ssm m)
