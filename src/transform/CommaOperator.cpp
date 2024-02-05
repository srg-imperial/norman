#include "CommaOperator.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::CommaOperatorConfig> transform::CommaOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::CommaOperatorConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformCommaOperator(CommaOperatorConfig const& config, Context& ctx,
                                                      clang::BinaryOperator& binop) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* lhs = binop.getLHS();
	clang::Expr* rhs = binop.getRHS();

	auto lhs_str = ctx.source_text(lhs->IgnoreParens()->getSourceRange());
	auto rhs_str = ctx.source_text(rhs->IgnoreParens()->getSourceRange());

	if(!lhs->HasSideEffects(*ctx.astContext)) {
		return {fmt::format("({})", rhs_str)};
	}

	return {fmt::format("({})", rhs_str), fmt::format("{};\n", lhs_str)};
}
