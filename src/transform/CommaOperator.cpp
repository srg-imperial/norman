#include "CommaOperator.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::CommaOperatorConfig> transform::CommaOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::CommaOperatorConfig>(v, [](auto& config, auto const& member) { return false; });
}

ExprTransformResult transform::transformCommaOperator(CommaOperatorConfig const& config, clang::ASTContext& astContext,
                                                      clang::BinaryOperator& binop) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* lhs = binop.getLHS();
	clang::Expr* rhs = binop.getRHS();

	auto lhs_str =
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(lhs->IgnoreParens()->getSourceRange()),
	                              astContext.getSourceManager(), astContext.getLangOpts());
	auto rhs_str =
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(rhs->IgnoreParens()->getSourceRange()),
	                              astContext.getSourceManager(), astContext.getLangOpts());

	if(!lhs->HasSideEffects(astContext)) {
		return {fmt::format("({})", rhs_str)};
	}

	return {fmt::format("({})", rhs_str), fmt::format("{};\n", lhs_str)};
}
