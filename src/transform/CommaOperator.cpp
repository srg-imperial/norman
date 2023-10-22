#include "CommaOperator.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<TransformationResult> transform::transformCommaOperator(clang::ASTContext* astContext,
                                                                      clang::BinaryOperator* binop) {
	clang::Expr* lhs = binop->getLHS();
	clang::Expr* rhs = binop->getRHS();

	auto lhs_str =
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(lhs->IgnoreParens()->getSourceRange()),
	                              astContext->getSourceManager(), astContext->getLangOpts());
	auto rhs_str =
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(rhs->IgnoreParens()->getSourceRange()),
	                              astContext->getSourceManager(), astContext->getLangOpts());

	if(!lhs->HasSideEffects(*astContext)) {
		return TransformationResult{fmt::format("({})", rhs_str), {}};
	}

	return TransformationResult{fmt::format("({})", rhs_str), fmt::format("{};\n", lhs_str)};
}
