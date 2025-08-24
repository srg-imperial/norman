#include "UnaryExprOrTypeTraitExpr.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <cassert>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

std::optional<transform::UnaryExprOrTypeTraitExprConfig>
transform::UnaryExprOrTypeTraitExprConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::UnaryExprOrTypeTraitExprConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExprConfig const& config,
                                                                 Context& ctx, clang::UnaryExprOrTypeTraitExpr& expr) {
	if(!config.enabled) {
		return {};
	}

	assert(expr.getType().getTypePtr()->isIntegerType() && "This expression should only yield integers");

	clang::Expr::EvalResult result;
	auto success = expr.EvaluateAsInt(result, *ctx.astContext);
	assert(success && "This expression should _always_ be constant evaluatable");
	assert(!result.hasSideEffects() && "This expression should not be able to have side effects");
	assert(!result.HasUndefinedBehavior && "This expression should never cause UB");

	return {result.Val.getAsString(*ctx.astContext, expr.getType())};
}
