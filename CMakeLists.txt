cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(fuse-example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall --pedantic -g")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(FUSE REQUIRED)
find_package(Boost REQUIRED filesystem)
include_directories(include ${FUSE_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

add_library(aqfs src/disk.cpp src/base.cpp src/inode.cpp src/dir.cpp src/runtime.cpp)

add_executable(aqfs.fuse src/fs.cpp)
target_link_libraries(aqfs.fuse aqfs ${FUSE_LIBRARIES} ${Boost_LIBRARIES})

add_executable(aqfs.mkfs src/mkfs.cpp)
target_link_libraries(aqfs.mkfs aqfs)
