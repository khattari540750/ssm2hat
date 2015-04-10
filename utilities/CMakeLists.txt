# ------------------------------------------------------------
# ssm utilities
# 
#  aotmatically build and install
#  ~/utilities
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set library and comple options 
#set(SSM_LIBS ${SSM_LIBS} ssm)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/utilities)


# install ssm-advance-player killssm psssm topssm
#ssm-advance-player
#killssm
#psssm
#topssm


# build and install lsssm
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(lsssm lsssm.c)
target_link_libraries(lsssm ${SSM_LIBS})
install(TARGETS lsssm DESTINATION bin)


# build and install ssm-logger
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-logger ssm-logger.cpp)
target_link_libraries(ssm-logger ${SSM_LIBS} m)
install(TARGETS ssm-logger DESTINATION bin)


# build and install ssm-player
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-player ssm-player.cpp)
target_link_libraries(ssm-player ${SSM_LIBS} m)
install(TARGETS ssm-player DESTINATION bin)


# build and install ssm-transporter
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm-transporter ssm-transporter.c)
target_link_libraries(ssm-transporter ${SSM_LIBS} m pthread)
install(TARGETS ssm-transporter DESTINATION bin)


# build and install ssm-date
add_executable(ssm-date ssm-date.c)
target_link_libraries(ssm-date ${SSM_LIBS})
install(TARGETS ssm-date DESTINATION bin)


# build and install ssm-monitor
add_executable(ssm-monitor ssm-monitor.c)
target_link_libraries(ssm-monitor ${SSM_LIBS})
install(TARGETS ssm-monitor DESTINATION bin)


# build and install ssm-graph
add_executable(ssm-graph ssm-graph.cpp)
target_link_libraries(ssm-graph ${SSM_LIBS})
install(TARGETS ssm-graph DESTINATION bin)