cmake_minimum_required(VERSION 2.8.7)

# If there is already a target called metrics someone has already
# added this project. Will ignore.
if(TARGET metrics)
    return()
endif(TARGET metrics)

project(NCodeCommon)
option(METRICS_DISABLE_TESTS "If tests should be compiled or not" ON)
option(METRICS_DEBUG "A debug build" OFF)
option(METRICS_ASAN "Compile with ASAN on" OFF)
option(METRICS_TSAN "Compile with TSAN on" OFF)

set(METRICS_BASE_FLAGS "-g -std=c++11 -pedantic-errors -Winit-self -Woverloaded-virtual -Wuninitialized -Wall -Wextra -fno-exceptions")
set(METRICS_BASE_LD_FLAGS "")
if (METRICS_ASAN)
   set(METRICS_BASE_FLAGS "${METRICS_BASE_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(METRICS_BASE_LD_FLAGS "${METRICS_BASE_LD_FLAGS} -fsanitize=address")
endif()
if (METRICS_TSAN)
   set(METRICS_BASE_FLAGS "${METRICS_BASE_FLAGS} -fsanitize=thread -fno-omit-frame-pointer -fno-optimize-sibling-calls")
   set(METRICS_BASE_LD_FLAGS "${METRICS_BASE_LD_FLAGS} -fsanitize=thread")
endif()

if (METRICS_DEBUG)
  set(METRICS_BASE_FLAGS "${METRICS_BASE_FLAGS} -O0 -fno-omit-frame-pointer")
else()
  set(METRICS_BASE_FLAGS "${METRICS_BASE_FLAGS} -O3 -march=native -DNDEBUG")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${METRICS_BASE_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${METRICS_BASE_LD_FLAGS}")

SET(OLD_BUILD_TESTING ${BUILD_TESTING} INTERNAL)
SET(BUILD_TESTING False)
add_subdirectory(external/gflags)
SET(BUILD_TESTING ${OLD_BUILD_TESTING})

if (NOT METRICS_DISABLE_TESTS)
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
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})

PROTOBUF_GENERATE_CPP(PROTO_METRICS_SRCS PROTO_METRICS_HDRS src/metrics.proto)
set_property(SOURCE ${PROTO_METRICS_SRCS} APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-extended-offsetof")
set(METRICS_HEADER_FILES src/metrics.h src/metrics_parser.h ${PROTO_METRICS_HDRS})
add_library(metrics STATIC src/metrics.cc ${PROTO_METRICS_SRCS} ${METRICS_HEADER_FILES})
target_link_libraries(metrics ${PROTOBUF_LIBRARIES} ncode_common gflags)

add_library(metrics_parser src/metrics_parser.cc ${PROTO_METRICS_SRCS} ${METRICS_HEADER_FILES})
target_link_libraries(metrics_parser ${PROTOBUF_LIBRARIES} ncode_common)

if (NOT METRICS_DISABLE_TESTS)
  add_library(metrics_test_util STATIC src/metrics_test_util.cc)
  target_link_libraries(metrics_test_util metrics gtest)

  add_test_exec(metrics_test src/metrics_test.cc metrics metrics_parser metrics_test_util)
  add_test_exec(metrics_parser_test src/metrics_parser_test.cc metrics metrics_parser metrics_test_util)
endif()

add_executable(metrics_explore src/metrics_explore)
target_link_libraries(metrics_explore metrics_parser gflags)
