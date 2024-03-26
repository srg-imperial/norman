add_test(NAME _norman_exists COMMAND norman --version)

SET(TESTS
  "core/function selection"
  "core/multiple functions"
  "NullStmt/basic"
)

foreach(test IN ITEMS ${TESTS})
  add_test(NAME ${test} COMMAND ${CMAKE_SOURCE_DIR}/test/runner.sh $<TARGET_FILE:norman> ${ClangFormat} ${test})
endforeach()