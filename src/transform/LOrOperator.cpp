#include "LOrOperator.h"

#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::LOrOperatorConfig> transform::LOrOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::LOrOperatorConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformLOrOperator(LOrOperatorConfig const& config, Context& ctx,
                                                    clang::BinaryOperator& binop) {
	if(!config.enabled) {
		return {};
	}

	auto lhs = binop.getLHS()->IgnoreParens();
	auto rhs = binop.getRHS()->IgnoreParens();

	auto lhs_str = ctx.source_text(lhs->getSourceRange());
	auto rhs_str = ctx.source_text(rhs->getSourceRange());

	if(bool lhs_val; lhs->EvaluateAsBooleanCondition(lhs_val, *ctx.astContext)) {
		if(lhs_val) {
			if(lhs->HasSideEffects(*ctx.astContext)) {
				return {std::format("({})", lhs_str)};
			} else {
				return {std::format("(1)")};
			}
		} else {
			if(lhs->HasSideEffects(*ctx.astContext)) {
				if(bool rhs_val;
				   !rhs->HasSideEffects(*ctx.astContext) && rhs->EvaluateAsBooleanCondition(rhs_val, *ctx.astContext)) {
					if(rhs_val) {
						return {std::format("(1)"), std::format("{};", lhs_str)};
					} else {
						return {std::format("({})", lhs_str)};
					}
				} else {
					return {std::format("({})", rhs_str), std::format("{};", lhs_str)};
				}
			} else {
				return {std::format("({})", rhs_str)};
			}
		}
	}

	std::string var_name = ctx.uid("LOr");

	if(bool rhs_val; rhs->EvaluateAsBooleanCondition(rhs_val, *ctx.astContext)) {
		if(rhs_val) {
			if(rhs->HasSideEffects(*ctx.astContext)) {
				return {std::format("(1)"), std::format("{};\n{};", lhs_str, rhs_str)};
			} else {
				return {std::format("(1)"), std::format("{};", lhs_str)};
			}
		} else {
			if(rhs->HasSideEffects(*ctx.astContext)) {
				auto to_hoist = std::format("_Bool {} = ({});\n{};", var_name, lhs_str, rhs_str);
				return {std::move(var_name), std::move(to_hoist)};
			} else {
				return {std::format("({})", lhs_str)};
			}
		}
	}

	auto to_hoist =
	  std::format("_Bool {} = ({});\nif(!{}) {{\n{} = ({});\n}}", var_name, lhs_str, var_name, var_name, rhs_str);
	return {std::move(var_name), std::move(to_hoist)};
}
