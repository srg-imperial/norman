cmake_minimum_required(VERSION 3.14)
project(norman)

include(FetchContent)

find_package(LLVM REQUIRED CONFIG HINTS "${LLVM_DIR}")
message(STATUS "LLVM directory ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Clang REQUIRED CONFIG HINTS "${LLVM_DIR}")

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.1.1)
  
FetchContent_MakeAvailable(fmtlib)

find_package(RapidJSON REQUIRED CONFIG)


# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

add_subdirectory("src")