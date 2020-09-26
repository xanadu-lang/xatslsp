cmake_minimum_required (VERSION 3.18 FATAL_ERROR)

# fetch external dependencies
include(FetchContent)
FetchContent_Declare(jsonh
  GIT_REPOSITORY https://github.com/sheredom/json.h.git
  GIT_TAG eeb1c14)
FetchContent_Declare(uriparse
  GIT_REPOSITORY https://github.com/whiler/uriparse.git
  GIT_TAG d48d43d)
FetchContent_Declare(uriencode
  GIT_REPOSITORY https://github.com/dnmfarrell/URI-Encode-C.git
  GIT_TAG 21ad52e)

FetchContent_MakeAvailable(jsonh uriparse uriencode)

project (XATSLS)

# uriparse does not supply a CMakeLists.txt file,
# so define a library yourself
add_library(uriparse
  ${uriparse_SOURCE_DIR}/src/uriparse.c
  ${uriparse_SOURCE_DIR}/src/uriparse.h)
# uriencode does not supply a CMakeLists.txt file,
# so define a library yourself
add_library(uriencode
  ${uriencode_SOURCE_DIR}/src/uri_encode.c
  ${uriencode_SOURCE_DIR}/src/uri_encode.h)

include_directories(
  ${jsonh_SOURCE_DIR}
  ${uriparse_SOURCE_DIR}/src
  ${uriencode_SOURCE_DIR}/src)

add_subdirectory (src)

enable_testing ()
add_subdirectory (tests)
