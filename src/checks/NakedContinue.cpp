#include "NakedContinue.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace checks {
	namespace {
		struct NakedContinue : clang::RecursiveASTVisitor<NakedContinue> {
			clang::ContinueStmt* continueStmt{};

			bool TraverseContinueStmt(clang::ContinueStmt* continueStmt) {
				this->continueStmt = continueStmt;
				return false;
			}

			bool TraverseWhileStmt(clang::WhileStmt*) { return true; }
			bool TraverseDoStmt(clang::DoStmt*) { return true; }
			bool TraverseForStmt(clang::ForStmt*) { return true; }
		};
	} // namespace

	clang::ContinueStmt* naked_continue(clang::Stmt* stmt) {
		NakedContinue visitor;
		visitor.TraverseStmt(stmt);
		return visitor.continueStmt;
	}
} // namespace checks
