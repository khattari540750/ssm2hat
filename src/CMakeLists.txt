# ------------------------------------------------------------
# libssm and ssm-coodinator
# 
#  aotmatically build and install
#  ~/src directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")
#set(CMAKE_C_FLAGS "-Werror")
#set(CMAKE_CXX_FLAGS "-Werror")


# build and install libssm static library
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(ssmStatic STATIC libssm2hat_shm.c libssm2hat_time.c libssm2hat.c)
target_link_libraries (ssmStatic pthread)
set_target_properties(ssmStatic PROPERTIES OUTPUT_NAME ssm2hat)
install(TARGETS ssmStatic DESTINATION lib)


# build and install libssm share library
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(ssmShared SHARED libssm2hat_shm.c libssm2hat_time.c libssm2hat.c)
target_link_libraries (ssmShared pthread)
set_target_properties(ssmShared PROPERTIES OUTPUT_NAME ssm2hat)
install(TARGETS ssmShared DESTINATION lib)


# build and install ssm-coodinator
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_executable(ssm_coordinator ssm_coordinator.cpp)
set(SSM_LIBS ${SSM_LIBS} ssmStatic)
target_link_libraries(ssm_coordinator ${SSM_LIBS})
install(TARGETS ssm_coordinator DESTINATION bin)