# ------------------------------------------------------------
#  libssm and ssm-coodinator CMake file for ssm
#  
#    auotmatically build
#  ~/src directory
# ------------------------------------------------------------


# set include directory
include_directories(${PROJECT_SOURCE_DIR}/include)


# set ALL COMPLE OPTIONS 
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")
#set(CMAKE_C_FLAGS "-Werror")
#set(CMAKE_CXX_FLAGS "-Werror")


# generate and install libssm static library
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(ssmStatic STATIC libssm-shm.c libssm-time.c libssm.c)
target_link_libraries (ssmStatic pthread)
set_target_properties(ssmStatic PROPERTIES OUTPUT_NAME ssm)
install(TARGETS ssmStatic DESTINATION lib)


# generate and install libssm share library
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_library(ssmShared SHARED libssm-shm.c libssm-time.c libssm.c)
target_link_libraries (ssmShared pthread)
set_target_properties(ssmShared PROPERTIES OUTPUT_NAME ssm)
install(TARGETS ssmShared DESTINATION lib)


# generate and install ssm-coodinator
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src)
add_executable(ssm-coordinator ssm-coordinator.cpp)
set(SSM_LIBS ${SSM_LIBS} ssmStatic)
target_link_libraries(ssm-coordinator ${SSM_LIBS})
install(TARGETS ssm-coordinator DESTINATION bin)