#pragma once

#include <clang/AST/AST.h>

namespace checks {
	clang::BreakStmt* naked_break(clang::Stmt* stmt);
} // namespace checks
