#pragma once

#include <clang/AST/AST.h>

namespace checks {
	clang::ContinueStmt* naked_continue(clang::Stmt& stmt);
} // namespace checks
