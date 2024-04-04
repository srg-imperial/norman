add_test(NAME _norman_exists COMMAND norman --version)

# All short tests are found by cmake
file(GLOB_RECURSE SHORT_TESTS CONFIGURE_DEPENDS "*.test")

# All directory tests are found by cmake, based on the location of a file called `input.c`
file(GLOB_RECURSE INPUT_C_FILES CONFIGURE_DEPENDS "input.c")
set(DIR_TESTS "")
foreach(f IN ITEMS ${INPUT_C_FILES})
  cmake_path(GET f PARENT_PATH d)
  LIST(APPEND DIR_TESTS ${d})
endforeach()

foreach(test IN ITEMS ${SHORT_TESTS} ${DIR_TESTS})
  add_test(
    NAME ${test}
    COMMAND ${CMAKE_SOURCE_DIR}/test/runner.py
      --norman $<TARGET_FILE:norman>
      --clang-format ${ClangFormat}
      ${test}
  )
endforeach()