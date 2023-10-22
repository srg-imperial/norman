#include "NakedBreak.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace checks {
	namespace {
		struct NakedBreak : clang::RecursiveASTVisitor<NakedBreak> {
			clang::BreakStmt* breakStmt{};

			bool TraverseBreakStmt(clang::BreakStmt* breakStmt) {
				this->breakStmt = breakStmt;
				return false;
			}

			bool TraverseWhileStmt(clang::WhileStmt*) { return true; }
			bool TraverseDoStmt(clang::DoStmt*) { return true; }
			bool TraverseForStmt(clang::ForStmt*) { return true; }
			bool TraverseSwitchStmt(clang::SwitchStmt*) { return true; }
		};
	} // namespace

	clang::BreakStmt* naked_break(clang::Stmt* stmt) {
		NakedBreak visitor;
		visitor.TraverseStmt(stmt);
		return visitor.breakStmt;
	}
} // namespace checks
