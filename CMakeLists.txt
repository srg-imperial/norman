cmake_minimum_required(VERSION 3.28)
project(norman)

include(CTest)
include(FetchContent)

find_package(LLVM REQUIRED CONFIG HINTS "${LLVM_DIR}")
message(STATUS "LLVM directory ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Clang REQUIRED CONFIG HINTS "${LLVM_DIR}")

find_program(ClangFormat NAMES clang-format REQUIRED PATHS "${LLVM_BINARY_DIR}/bin" NO_DEFAULT_PATH)

find_package(RapidJSON REQUIRED CONFIG)


set(CMAKE_CXX_STANDARD 20 CACHE STRING "")

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_subdirectory("src")
add_subdirectory("test")
