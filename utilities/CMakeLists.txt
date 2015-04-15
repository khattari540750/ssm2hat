# ------------------------------------------------------------
# ssm utilities
# 
#  aotmatically build and install
#  ~/utilities
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/src)


# set library and comple options 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")


# add the executable files
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/utilities)


# install ssm_advance_player script
install(FILES ssm_advance_player DESTINATION bin 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


# install killssm script
install(FILES killssm DESTINATION bin 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# install psssm script
install(FILES psssm DESTINATION bin 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


# install topssm script
install(FILES topssm DESTINATION bin 
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


# build and install lsssm
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(lsssm lsssm.c)
target_link_libraries(lsssm ssm2hat)
install(TARGETS lsssm DESTINATION bin)


# build and install ssm_logger
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm_logger ssm_logger.cpp)
target_link_libraries(ssm_logger ssm2hat m)
install(TARGETS ssm_logger DESTINATION bin)


# build and install ssm_player
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm_player ssm_player.cpp)
target_link_libraries(ssm_player ssm2hat m)
install(TARGETS ssm_player DESTINATION bin)


# build and install ssm_transporter
#set(CMAKE_C_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
#set(CMAKE_CXX_FLAGS "-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
add_executable(ssm_transporter ssm_transporter.c)
target_link_libraries(ssm_transporter ssm2hat m pthread)
install(TARGETS ssm_transporter DESTINATION bin)


# build and install ssm_date
add_executable(ssm_date ssm_date.c)
target_link_libraries(ssm_date ssm2hat)
install(TARGETS ssm_date DESTINATION bin)


# build and install ssm_monitor
add_executable(ssm_monitor ssm_monitor.c)
target_link_libraries(ssm_monitor ssm2hat)
install(TARGETS ssm_monitor DESTINATION bin)


# build and install ssm_graph
add_executable(ssm_graph ssm_graph.cpp)
target_link_libraries(ssm_graph ssm2hat)
install(TARGETS ssm_graph DESTINATION bin)