#include "LOrOperator.h"

#include "../util/UId.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

std::optional<transform::LOrOperatorConfig> transform::LOrOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::LOrOperatorConfig>(v, [](auto& config, auto const& member) { return false; });
}

ExprTransformResult transform::transformLOrOperator(LOrOperatorConfig const& config, clang::ASTContext& astContext,
                                                    clang::BinaryOperator& binop) {
	if(!config.enabled) {
		return {};
	}

	auto lhs = binop.getLHS()->IgnoreParens();
	auto rhs = binop.getRHS()->IgnoreParens();

	auto lhs_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(lhs->getSourceRange()),
	                                           astContext.getSourceManager(), astContext.getLangOpts());

	auto rhs_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(rhs->getSourceRange()),
	                                           astContext.getSourceManager(), astContext.getLangOpts());

	if(bool lhs_val; lhs->EvaluateAsBooleanCondition(lhs_val, astContext)) {
		if(lhs_val) {
			if(lhs->HasSideEffects(astContext)) {
				return {fmt::format("({})", lhs_str)};
			} else {
				return {fmt::format("(1)")};
			}
		} else {
			if(lhs->HasSideEffects(astContext)) {
				if(bool rhs_val; !rhs->HasSideEffects(astContext) && rhs->EvaluateAsBooleanCondition(rhs_val, astContext)) {
					if(rhs_val) {
						return {fmt::format("(1)"), fmt::format("{};", lhs_str)};
					} else {
						return {fmt::format("({})", lhs_str)};
					}
				} else {
					return {fmt::format("({})", rhs_str), fmt::format("{};", lhs_str)};
				}
			} else {
				return {fmt::format("({})", rhs_str)};
			}
		}
	}

	std::string var_name = util::uid(astContext, "_LOr");

	if(bool rhs_val; rhs->EvaluateAsBooleanCondition(rhs_val, astContext)) {
		if(rhs_val) {
			if(rhs->HasSideEffects(astContext)) {
				return {fmt::format("(1)"), fmt::format("{};\n{};", lhs_str, rhs_str)};
			} else {
				return {fmt::format("(1)"), fmt::format("{};", lhs_str)};
			}
		} else {
			if(rhs->HasSideEffects(astContext)) {
				auto to_hoist = fmt::format("_Bool {} = ({});\n{};", var_name, lhs_str, rhs_str);
				return {std::move(var_name), std::move(to_hoist)};
			} else {
				return {fmt::format("({})", lhs_str)};
			}
		}
	}

	auto to_hoist =
	  fmt::format("_Bool {} = ({});\nif(!{}) {{\n{} = ({});\n}}", var_name, lhs_str, var_name, var_name, rhs_str);
	return {std::move(var_name), std::move(to_hoist)};
}
