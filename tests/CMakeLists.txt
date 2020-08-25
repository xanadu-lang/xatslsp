include_directories (${TEST_SOURCE_DIR}/src)

add_executable (json_rpc_tests json_rpc_tests.c)
target_link_libraries (json_rpc_tests
  PRIVATE
  json_rpc
  picohttpparser
  )
add_test (NAME json_rpc_tests COMMAND json_rpc_tests)
