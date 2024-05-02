# Add a "test" that builds the sources
add_test(_test_build
  "${CMAKE_COMMAND}"
  --build "${CMAKE_BINARY_DIR}"
  --config "$<CONFIG>"
  --target norman
)
set_tests_properties(_test_build PROPERTIES FIXTURES_SETUP test_fixture)

add_test(NAME _norman_exists COMMAND norman --version)
set_tests_properties(_norman_exists PROPERTIES FIXTURES_REQUIRED test_fixture)

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
  cmake_path(RELATIVE_PATH test OUTPUT_VARIABLE test_name)
  add_test(
    NAME ${test_name}
    COMMAND ${CMAKE_SOURCE_DIR}/test/runner.py
      --norman $<TARGET_FILE:norman>
      --clang-format ${ClangFormat}
      ${test}
  )
  set_tests_properties(${test_name} PROPERTIES FIXTURES_REQUIRED test_fixture)
endforeach()