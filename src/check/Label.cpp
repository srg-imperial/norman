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

	/// @brief Searches the given statement for a label
	/// @param stmt The statement to search for a label
	/// @return A pointer to a label if one exists, and a nullpointer otherwise
	clang::LabelStmt* label(clang::Stmt& stmt) {
		Label visitor;
		visitor.TraverseStmt(&stmt);
		return visitor.labelStmt;
	}
} // namespace checks
