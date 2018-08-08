project (minac)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-g -Wall -lpthread")
set(CMAKE_VERBOSE_MAKEFILE 1)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(PkgConfig) 
pkg_check_modules(LIBS libevent_pthreads libevent zlog trio)
link_directories(${LIBS_LIBRARY_DIRS})
include_directories(${LIBS_INCLUDE_DIRS}) 

ADD_SUBDIRECTORY(src/core)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/core)
link_directories(${PROJECT_BINARY_DIR}/lib)

aux_source_directory(src/test SRC_LIST)
add_executable(minac_test ${SRC_LIST})
TARGET_LINK_LIBRARIES(minac_test minac ${LIBS_LIBRARIES})
