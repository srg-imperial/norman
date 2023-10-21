#pragma once

#include <clang/AST/AST.h>
#include <llvm/Support/Casting.h>

namespace checks {
	inline bool isSimpleValue(clang::Expr* arg) {
		return llvm::isa<clang::DeclRefExpr>(arg) || llvm::isa<clang::IntegerLiteral>(arg) ||
		       llvm::isa<clang::FloatingLiteral>(arg) || llvm::isa<clang::CharacterLiteral>(arg) ||
		       llvm::isa<clang::StringLiteral>(arg) || llvm::isa<clang::ImaginaryLiteral>(arg);
	}
} // namespace checks
