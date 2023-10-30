#include "ParenExpr.h"

#include "../check/SimpleValue.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::ParenExprConfig> transform::ParenExprConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::ParenExprConfig>(v, [](auto& config, auto const& member) { return false; });
}

std::optional<TransformationResult>
transform::transformParenExpr(ParenExprConfig const& config, clang::ASTContext& astContext, clang::ParenExpr& pexpr) {
	if(!config.enabled) {
		return {};
	}

	auto stripped = pexpr.IgnoreParens();

	if(checks::isSimpleValue(*stripped)) {
		return TransformationResult{
		  fmt::format(" {} ", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stripped->getSourceRange()),
		                                                  astContext.getSourceManager(), astContext.getLangOpts())),
		  {}};
	} else if(stripped != pexpr.getSubExpr()) {
		return TransformationResult{
		  fmt::format("({})", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stripped->getSourceRange()),
		                                                  astContext.getSourceManager(), astContext.getLangOpts())),
		  {}};
	} else {
		return {};
	}
}
