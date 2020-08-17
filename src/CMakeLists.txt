add_library (json_rpc json_rpc.c json_rpc.h)

add_executable (xatslsp xatslsp_main.c language_server.c)
target_link_libraries (xatslsp json_rpc picohttpparser)

install(TARGETS xatslsp DESTINATION bin)
