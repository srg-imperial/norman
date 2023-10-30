#pragma once

#include <clang/AST/AST.h>

namespace checks {
	clang::LabelStmt* label(clang::Stmt& stmt);
} // namespace checks
