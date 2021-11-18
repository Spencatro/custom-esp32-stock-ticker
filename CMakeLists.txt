# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(FINNHUB_API_KEY $ENV{FINNHUB_API_KEY})
configure_file(main/api_config.h.in api_config.h)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories (${CMAKE_BINARY_DIR})
project(blue_grid)
