cmake_minimum_required(VERSION 3.10)
project(mpi_help)

find_package(MPI REQUIRED)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

set(CMAKE_CXX_STANDARD 14)


add_executable(mpi_helper main.cpp)
target_link_libraries(mpi_helper ${MPI_CXX_LIBRARIES})

enable_testing()
add_subdirectory(test)
