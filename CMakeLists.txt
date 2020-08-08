cmake_minimum_required (VERSION 3.11 FATAL_ERROR)

# fetch external dependencies
include(FetchContent)
FetchContent_Declare(jsonh
  GIT_REPOSITORY https://github.com/sheredom/json.h.git
  GIT_TAG eeb1c14)
FetchContent_Declare(picohttpparser
  GIT_REPOSITORY https://github.com/h2o/picohttpparser.git
  GIT_TAG 8ba2328)

FetchContent_MakeAvailable(jsonh picohttpparser)

project (TEST)

# picohttpparser does not supply a CMakeLists.txt file,
# so define a library yourself
add_library(picohttpparser
  ${picohttpparser_SOURCE_DIR}/picohttpparser.c
  ${picohttpparser_SOURCE_DIR}/picohttpparser.h)

include_directories(${jsonh_SOURCE_DIR} ${picohttpparser_SOURCE_DIR})

add_subdirectory (src)

enable_testing ()
add_subdirectory (tests)
