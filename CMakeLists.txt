cmake_minimum_required(VERSION 2.8.7)

# If there is already a target called ncode_metrics someone has already
# added this project. Will ignore.
if(TARGET ncode_metrics)
    return()
endif(TARGET ncode_metrics)

project(NCodeCommon)
option(NCODE_METRICS_DISABLE_TESTS "If tests should be compiled or not" ON)
option(NCODE_METRICS_DEBUG "A debug build" OFF)
option(NCODE_METRICS_ASAN "Compile with ASAN on" OFF)
option(NCODE_METRICS_TSAN "Compile with TSAN on" OFF)

set(NCODE_METRICS_BASE_FLAGS "-g -std=c++11 -pedantic-errors -Winit-self -Woverloaded-virtual -Wuninitialized -Wall -Wextra -fno-exceptions")
set(NCODE_METRICS_BASE_LD_FLAGS "")
if (NCODE_METRICS_ASAN)
   set(NCODE_METRICS_BASE_FLAGS "${NCODE_METRICS_BASE_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(NCODE_METRICS_BASE_LD_FLAGS "${NCODE_METRICS_BASE_LD_FLAGS} -fsanitize=address")
endif()
if (NCODE_METRICS_TSAN)
   set(NCODE_METRICS_BASE_FLAGS "${NCODE_METRICS_BASE_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(NCODE_METRICS_BASE_LD_FLAGS "${NCODE_METRICS_BASE_LD_FLAGS} -fsanitize=thread")
endif()

if (NCODE_METRICS_DEBUG)
  set(NCODE_METRICS_BASE_FLAGS "${NCODE_METRICS_BASE_FLAGS} -O0 -fno-omit-frame-pointer")
else()
  set(NCODE_METRICS_BASE_FLAGS "${NCODE_METRICS_BASE_FLAGS} -O3 -march=native -DNDEBUG")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NCODE_METRICS_BASE_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${NCODE_METRICS_BASE_LD_FLAGS}")

if (NOT NCODE_METRICS_DISABLE_TESTS)
   include(CTest)
   add_subdirectory(external/googletest)
   macro(add_test_exec name src_file deps)
     add_executable(${name} ${src_file})
     target_link_libraries(${name} gtest gmock_main ${deps} ${ARGN})
     add_test(NAME ${name} COMMAND ${name})
   endmacro(add_test_exec)
endif()

find_package(Protobuf REQUIRED)

add_subdirectory(external/ncode_common)
add_subdirectory(external/ncode_web)
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

PROTOBUF_GENERATE_CPP(PROTO_METRICS_SRCS PROTO_METRICS_HDRS src/metrics.proto)
set_property(SOURCE ${PROTO_METRICS_SRCS} APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-extended-offsetof")
set(METRICS_HEADER_FILES src/metrics.h src/metrics_parser.h ${PROTO_METRICS_HDRS})
add_library(ncode_metrics STATIC src/metrics.cc src/metrics_parser.cc ${PROTO_METRICS_SRCS} ${METRICS_HEADER_FILES})
target_link_libraries(ncode_metrics ${PROTOBUF_LIBRARIES} ncode_common ncode_web)

if (NOT NCODE_METRICS_DISABLE_TESTS)
  add_library(metrics_test_util STATIC src/metrics_test_util.cc)
  target_link_libraries(metrics_test_util ncode_metrics gtest)

  add_test_exec(metrics_test src/metrics_test.cc ncode_metrics metrics_test_util)
  add_test_exec(metrics_parser_test src/metrics_parser_test.cc ncode_metrics metrics_test_util)
endif()
