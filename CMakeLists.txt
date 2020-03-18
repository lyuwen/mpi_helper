cmake_minimum_required(VERSION 3.14)
project(mpi_help)

find_package(MPI REQUIRED)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

set(CMAKE_CXX_STANDARD 14)


add_executable(mpi_help main.cpp)
target_link_libraries(mpi_help ${MPI_CXX_LIBRARIES})

enable_testing()
add_test(NAME test_mpi
  COMMAND mpirun --oversubscribe -n 2 ./mpi_help -v ${CMAKE_CURRENT_SOURCE_DIR}/test_input)
set_tests_properties(test_mpi PROPERTIES
  ENVIRONMENT PMIX_MCA_gds=^ds12)
