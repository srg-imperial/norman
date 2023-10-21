#include "LAndOperator.h"

#include "utils/UId.h"

#include "utils/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <string>

std::optional<TransformationResult> transformLAndOperator(clang::ASTContext* astContext, clang::BinaryOperator* binop) {
	auto lhs = binop->getLHS()->IgnoreParens();
	auto rhs = binop->getRHS()->IgnoreParens();

	auto lhs_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(lhs->getSourceRange()),
	                                           astContext->getSourceManager(), astContext->getLangOpts());

	auto rhs_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(rhs->getSourceRange()),
	                                           astContext->getSourceManager(), astContext->getLangOpts());

	if(bool lhs_val; lhs->EvaluateAsBooleanCondition(lhs_val, *astContext)) {
		if(lhs_val) {
			if(lhs->HasSideEffects(*astContext)) {
				if(bool rhs_val; !rhs->HasSideEffects(*astContext) && rhs->EvaluateAsBooleanCondition(rhs_val, *astContext)) {
					if(rhs_val) {
						return TransformationResult{fmt::format("({})", lhs_str), {}};
					} else {
						return TransformationResult{fmt::format("(0)"), fmt::format("{};", lhs_str)};
					}
				} else {
					return TransformationResult{fmt::format("({})", rhs_str), fmt::format("{};", lhs_str)};
				}
			} else {
				return TransformationResult{fmt::format("({})", rhs_str), {}};
			}
		} else {
			if(lhs->HasSideEffects(*astContext)) {
				return TransformationResult{fmt::format("({})", lhs_str), {}};
			} else {
				return TransformationResult{fmt::format("(0)"), {}};
			}
		}
	}

	std::string var_name = uid(astContext, "_LAnd");

	if(bool rhs_val; rhs->EvaluateAsBooleanCondition(rhs_val, *astContext)) {
		if(rhs_val) {
			if(rhs->HasSideEffects(*astContext)) {
				auto to_hoist = fmt::format("_Bool {} = ({});\n{};", var_name, lhs_str, rhs_str);
				return TransformationResult{std::move(var_name), std::move(to_hoist)};
			} else {
				return TransformationResult{fmt::format("({})", lhs_str), {}};
			}
		} else {
			if(rhs->HasSideEffects(*astContext)) {
				return TransformationResult{fmt::format("(0)"), fmt::format("{};\n{};", lhs_str, rhs_str)};
			} else {
				return TransformationResult{fmt::format("(0)"), fmt::format("{};", lhs_str)};
			}
		}
	}

	auto to_hoist =
	  fmt::format("_Bool {} = ({});\nif({}) {{\n{} = ({});\n}}", var_name, lhs_str, var_name, var_name, rhs_str);
	return TransformationResult{std::move(var_name), std::move(to_hoist)};
}
