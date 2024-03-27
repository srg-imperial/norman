add_test(NAME _norman_exists COMMAND norman --version)

# All short tests are found by cmake
file(GLOB_RECURSE TESTS CONFIGURE_DEPENDS "*.test")

# Directory-style tests can be appended to the list here
#list(APPEND TESTS
#  "core/some test"
#)

foreach(test IN ITEMS ${TESTS})
  add_test(
    NAME ${test}
    COMMAND ${CMAKE_SOURCE_DIR}/test/runner.py
      --norman $<TARGET_FILE:norman>
      --clang-format ${ClangFormat}
      ${test}
  )
endforeach()