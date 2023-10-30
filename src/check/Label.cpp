#include "Label.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace checks {
	namespace {
		struct Label : clang::RecursiveASTVisitor<Label> {
			clang::LabelStmt* labelStmt{};

			bool TraverseLabelStmt(clang::LabelStmt* labelStmt) {
				this->labelStmt = labelStmt;
				return false;
			}
		};
	} // namespace

	clang::LabelStmt* label(clang::Stmt& stmt) {
		Label visitor;
		visitor.TraverseStmt(&stmt);
		return visitor.labelStmt;
	}
} // namespace checks
