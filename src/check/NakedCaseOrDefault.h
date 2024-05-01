#pragma once

#include <clang/AST/AST.h>

namespace checks {
	clang::Stmt* naked_case_or_default(clang::Stmt& stmt);
} // namespace checks
