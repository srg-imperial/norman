#include "Reference.h"

#include <clang/AST/RecursiveASTVisitor.h>

namespace checks {
	namespace {
		struct Reference : clang::RecursiveASTVisitor<Reference> {
			clang::Decl* decl;
			clang::DeclRefExpr* declRef{};

			Reference(clang::Decl* decl)
			  : decl(decl) { }

			bool TraverseDeclRefExpr(clang::DeclRefExpr* declRef) {
				if(declRef->getDecl() == decl) {
					this->declRef = declRef;
					return false;
				} else {
					return true;
				}
			}
		};
	} // namespace

	clang::DeclRefExpr* reference(clang::Stmt* stmt, clang::Decl* decl) {
		Reference visitor{decl};
		visitor.TraverseStmt(stmt);
		return visitor.declRef;
	}
} // namespace checks
