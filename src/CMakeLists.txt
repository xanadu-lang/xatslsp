add_library (json_rpc json_rpc.c json_rpc.h)
add_library (text_buffer text_buffer.c text_buffer.h)
add_library (file_system file_system.c file_system.h)
target_link_libraries (file_system uriparse uriencode text_buffer)

add_executable (xatsls xatsls_main.c language_server.c)
target_link_libraries (xatsls json_rpc file_system)

install(TARGETS xatsls DESTINATION bin)
