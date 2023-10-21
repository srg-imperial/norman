#include "StmtExpr.h"

#include "utils/UId.h"

#include "utils/fmtlib_clang.h"
#include "utils/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>

// format_to
std::optional<TransformationResult> transformStmtExpr(clang::ASTContext* astContext, clang::StmtExpr* stexpr) {
	clang::CompoundStmt* Stmts = stexpr->getSubStmt();
	clang::QualType stexpr_type = stexpr->getType();

	if(stexpr_type->isVoidType()) {
		return TransformationResult{
		  "((void)0)", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(Stmts->getSourceRange()),
		                                           astContext->getSourceManager(), astContext->getLangOpts())
		                 .str()};
	}

	std::string to_hoist;
	auto out = std::back_inserter(to_hoist);

	std::string var_name = uid(astContext, "_StmtExpr");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  *astContext, astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &astContext->Idents.get(var_name), stexpr_type, nullptr, clang::StorageClass::SC_None);
	out = fmt::format_to(out, "{};\n{{", *vd);

	for(clang::CompoundStmt::body_iterator S = Stmts->body_begin(), end = std::prev(Stmts->body_end()); S != end; ++S) {
		out = fmt::format_to(out, "{};\n",
		                     clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange((*S)->getSourceRange()),
		                                                 astContext->getSourceManager(), astContext->getLangOpts()));
	}

	out = fmt::format_to(
	  out, "{}=({});\n}}", var_name,
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(Stmts->body_back()->getSourceRange()),
	                              astContext->getSourceManager(), astContext->getLangOpts()));
	return TransformationResult{std::move(var_name), std::move(to_hoist)};
}
