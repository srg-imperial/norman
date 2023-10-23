#pragma once

#include <clang/AST/AST.h>

namespace checks {
	clang::DeclRefExpr* reference(clang::Stmt* stmt, clang::Decl* decl);
} // namespace checks
