include_directories (../src)

add_executable (json_rpc_tests json_rpc_tests.c)
target_link_libraries (json_rpc_tests
  PRIVATE
  json_rpc
  )
add_test (NAME json_rpc_tests COMMAND $<TARGET_FILE:json_rpc_tests>)

add_executable (file_system_tests file_system_tests.c)
target_link_libraries (file_system_tests
  PRIVATE
  uriparse
  uriencode
  file_system
  )
add_test (NAME file_system_tests COMMAND $<TARGET_FILE:file_system_tests>)

add_executable (strslice_tests strslice_tests.c)
add_test (NAME strslice_tests COMMAND $<TARGET_FILE:strslice_tests>)

add_executable (text_buffer_tests text_buffer_tests.c)
add_custom_command(
  TARGET text_buffer_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/tests/text_buffer_tests.txt
  ${CMAKE_CURRENT_BINARY_DIR}/text_buffer_tests.txt)
add_custom_command(
  TARGET text_buffer_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/tests/text_buffer_tests.txt.ok
  ${CMAKE_CURRENT_BINARY_DIR}/text_buffer_tests.txt.ok)
target_link_libraries (text_buffer_tests PRIVATE text_buffer)
add_test (NAME text_buffer_tests COMMAND $<TARGET_FILE:text_buffer_tests>)
add_test (NAME text_buffer_compare
  COMMAND ${CMAKE_COMMAND} -E compare_files
  text_buffer_tests.txt.out
  text_buffer_tests.txt.ok)
