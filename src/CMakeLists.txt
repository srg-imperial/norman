add_executable(norman
  Norman.cpp
  Config.cpp
  Context.cpp
  check/Label.cpp
  check/NakedBreak.cpp
  check/NakedCaseOrDefault.cpp
  check/NakedContinue.cpp
  check/Reference.cpp
  transform/CallArg.cpp
  transform/CommaOperator.cpp
  transform/ConditionalOperator.cpp
  transform/DoStmt.cpp
  transform/ForStmt.cpp
  transform/IfStmt.cpp
  transform/LAndOperator.cpp
  transform/LOrOperator.cpp
  transform/ParenExpr.cpp
  transform/ReturnStmt.cpp
  transform/StmtExpr.cpp
  transform/StringLiteral.cpp
  transform/SwitchStmt.cpp
  transform/UnaryExprOrTypeTraitExpr.cpp
  transform/VarDecl.cpp
  transform/WhileStmt.cpp
)

target_link_libraries(norman PRIVATE
  clangASTMatchers
  clangBasic
  clangTooling
  fmt::fmt
)

install(TARGETS norman DESTINATION bin)