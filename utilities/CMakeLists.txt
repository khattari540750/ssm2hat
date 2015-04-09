# ------------------------------------------------------------
#  utilities CMake file for ssm
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
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/utilities)

## ssm-advance-player
## killssm
## psssm
## topssm


## lsssm
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(lsssm lsssm.c)
target_link_libraries(lsssm ${SSM_LIBS})

## ssm-logger
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-logger ssm-logger.cpp)
target_link_libraries(ssm-logger ${SSM_LIBS} m)

## ssm-player
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-player ssm-player.cpp)
target_link_libraries(ssm-player ${SSM_LIBS} m)

## ssm-transporter
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-transporter ssm-transporter.c)
target_link_libraries(ssm-transporter ${SSM_LIBS} m pthread)

## ssm-date
add_executable(ssm-date ssm-date.c)
target_link_libraries(ssm-date ${SSM_LIBS})

## ssm-monitor
add_executable(ssm-monitor ssm-monitor.c)
target_link_libraries(ssm-monitor ${SSM_LIBS})

## ssm-graph
add_executable(ssm-graph ssm-graph.cpp)
target_link_libraries(ssm-graph ${SSM_LIBS})
