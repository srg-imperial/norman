#pragma once

#include <clang/AST/AST.h>
#include <llvm/Support/Casting.h>

namespace checks {
	inline bool null_stmt(clang::Stmt* stmt) {
		if(!stmt || llvm::isa<clang::NullStmt>(stmt)) {
			return true;
		} else if(auto* cstmt = llvm::dyn_cast<clang::CompoundStmt>(stmt)) {
			for(auto child : cstmt->children()) {
				if(!null_stmt(child)) {
					return false;
				}
			}
			return true;
		}

		return false;
	}
} // namespace checks