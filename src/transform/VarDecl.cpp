#include "VarDecl.h"

#include "../check/Reference.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <iterator>
#include <string>

std::optional<std::string> transform::transformVarDecl(clang::ASTContext* astContext, clang::VarDecl* varDecl) {
	if(clang::Expr* init = varDecl->getInit()) {
		if(varDecl->isFileVarDecl() || varDecl->isStaticLocal()) {
			// in C file vars and static locals have to be constant initialized
		} else if(varDecl->isLocalVarDecl()) {
			if(checks::reference(init, varDecl)) {
				clang::VarDecl* vd = clang::VarDecl::Create(
				  *astContext, varDecl->getDeclContext(), clang::SourceLocation(), clang::SourceLocation(),
				  varDecl->getIdentifier(), varDecl->getType(), varDecl->getTypeSourceInfo(), varDecl->getStorageClass());

				return fmt::format("{};\n{} = ({});", *vd, varDecl->getName(),
				                   clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(init->getSourceRange()),
				                                               astContext->getSourceManager(), astContext->getLangOpts()));
			}
		}
	}
	return {};
}
