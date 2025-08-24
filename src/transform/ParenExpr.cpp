#include "ParenExpr.h"

#include "../check/SimpleValue.h"

#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::ParenExprConfig> transform::ParenExprConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::ParenExprConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformParenExpr(ParenExprConfig const& config, Context& ctx,
                                                  clang::ParenExpr& pexpr) {
	if(!config.enabled) {
		return {};
	}

	auto stripped = pexpr.IgnoreParens();

	if(checks::isSimpleValue(*stripped)) {
		return {std::format(" {} ", ctx.source_text(stripped->getSourceRange()))};
	} else if(stripped != pexpr.getSubExpr()) {
		return {std::format("({})", ctx.source_text(stripped->getSourceRange()))};
	} else {
		return {};
	}
}
