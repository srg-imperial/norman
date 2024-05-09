#include "LAndOperator.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <string>

std::optional<transform::LAndOperatorConfig> transform::LAndOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::LAndOperatorConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformLAndOperator(LAndOperatorConfig const& config, Context& ctx,
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
				if(bool rhs_val;
				   !rhs->HasSideEffects(*ctx.astContext) && rhs->EvaluateAsBooleanCondition(rhs_val, *ctx.astContext)) {
					if(rhs_val) {
						return {fmt::format("({})", lhs_str)};
					} else {
						return {fmt::format("(0)"), fmt::format("{};", lhs_str)};
					}
				} else {
					return {fmt::format("({})", rhs_str), fmt::format("{};", lhs_str)};
				}
			} else {
				return {fmt::format("({})", rhs_str)};
			}
		} else {
			if(lhs->HasSideEffects(*ctx.astContext)) {
				return {fmt::format("({})", lhs_str)};
			} else {
				return {fmt::format("(0)")};
			}
		}
	}

	std::string var_name = ctx.uid("LAnd");

	if(bool rhs_val; rhs->EvaluateAsBooleanCondition(rhs_val, *ctx.astContext)) {
		if(rhs_val) {
			if(rhs->HasSideEffects(*ctx.astContext)) {
				auto to_hoist = fmt::format("_Bool {} = ({});\n{};", var_name, lhs_str, rhs_str);
				return {std::move(var_name), std::move(to_hoist)};
			} else {
				return {fmt::format("({})", lhs_str)};
			}
		} else {
			if(rhs->HasSideEffects(*ctx.astContext)) {
				return {fmt::format("(0)"), fmt::format("{};\n{};", lhs_str, rhs_str)};
			} else {
				return {fmt::format("(0)"), fmt::format("{};", lhs_str)};
			}
		}
	}

	auto to_hoist =
	  fmt::format("_Bool {} = ({});\nif({}) {{\n{} = ({});\n}}", var_name, lhs_str, var_name, var_name, rhs_str);
	return {std::move(var_name), std::move(to_hoist)};
}
