add_executable(NormaliseExprTransform
  NormaliseExprTransform.cpp
  checks/Label.cpp
  checks/NakedBreak.cpp
  checks/NakedContinue.cpp
  CallArg.cpp
  CommaOperator.cpp
  ConditionalOperator.cpp
  DoStmt.cpp
  ForStmt.cpp
  IfStmt.cpp
  LAndOperator.cpp
  LOrOperator.cpp
  ParenExpr.cpp
  ReturnStmt.cpp
  StmtExpr.cpp
  StringLiteral.cpp
  SwitchStmt.cpp
  WhileStmt.cpp
)

target_link_libraries(NormaliseExprTransform PRIVATE
  clangASTMatchers
  clangBasic
  clangTooling
  fmt::fmt
)

install(TARGETS NormaliseExprTransform RUNTIME DESTINATION bin)