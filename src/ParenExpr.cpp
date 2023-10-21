#include "ParenExpr.h"

#include "checks/SimpleValue.h"

#include "utils/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<TransformationResult> transformParenExpr(clang::ASTContext* astContext, clang::ParenExpr* pexpr) {
	auto stripped = pexpr->IgnoreParens();

	if(checks::isSimpleValue(stripped)) {
		return TransformationResult{
		  fmt::format(" {} ", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stripped->getSourceRange()),
		                                                  astContext->getSourceManager(), astContext->getLangOpts())),
		  {}};
	} else if(stripped != pexpr->getSubExpr()) {
		return TransformationResult{
		  fmt::format("({})", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stripped->getSourceRange()),
		                                                  astContext->getSourceManager(), astContext->getLangOpts())),
		  {}};
	} else {
		return {};
	}
}
