add_test(NAME test_mpi
  COMMAND mpirun --oversubscribe -n 1 ${CMAKE_BINARY_DIR}/mpi_helper -v ${CMAKE_CURRENT_SOURCE_DIR}/test_input)
set_tests_properties(test_mpi PROPERTIES
  ENVIRONMENT PMIX_MCA_gds=^ds12)
# elseif (CMAKE_SYSTEM_NAME EQUAL Linux)
# add_test(NAME test_mpi
#   COMMAND mpirun -n 1 ./mpi_help -v ${CMAKE_CURRENT_SOURCE_DIR}/test_input)
# endif ()

add_executable(test.x test.cpp)
target_link_libraries(test.x ${MPI_CXX_LIBRARIES})
