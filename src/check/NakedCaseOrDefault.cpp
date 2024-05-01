#include "NakedCaseOrDefault.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace checks {
	namespace {
		struct NakedCaseOrDefault : clang::RecursiveASTVisitor<NakedCaseOrDefault> {
			clang::Stmt* caseOrDefaultStmt{};

		public:
			bool TraverseCaseStmt(clang::CaseStmt* caseStmt) {
				caseOrDefaultStmt = caseStmt;
				return false;
			}

			bool TraverseDefaultStmt(clang::DefaultStmt* defaultStmt) {
				caseOrDefaultStmt = defaultStmt;
				return false;
			}

			bool TraverseSwitchStmt(clang::SwitchStmt*) { return true; }
		};
	} // namespace

	clang::Stmt* naked_case_or_default(clang::Stmt& stmt) {
		NakedCaseOrDefault visitor;
		visitor.TraverseStmt(&stmt);
		return visitor.caseOrDefaultStmt;
	}
} // namespace checks
