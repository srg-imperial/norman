#include "ConditionalOperator.h"

#include "../util/UId.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>

std::optional<transform::ConditionalOperatorConfig>
transform::ConditionalOperatorConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::ConditionalOperatorConfig>(
	  v, [](auto& config, auto const& member) { return false; });
}

std::optional<TransformationResult> transform::transformConditionalOperator(ConditionalOperatorConfig const& config,
                                                                            clang::ASTContext& astContext,
                                                                            clang::ConditionalOperator& cop) {
	if(!config.enabled) {
		return {};
	}

	auto const* cond = cop.getCond()->IgnoreParens();

	auto cexpr = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
	                                         astContext.getSourceManager(), astContext.getLangOpts());
	auto texpr = clang::Lexer::getSourceText(
	  clang::CharSourceRange::getTokenRange(cop.getTrueExpr()->IgnoreParens()->getSourceRange()),
	  astContext.getSourceManager(), astContext.getLangOpts());
	auto fexpr = clang::Lexer::getSourceText(
	  clang::CharSourceRange::getTokenRange(cop.getFalseExpr()->IgnoreParens()->getSourceRange()),
	  astContext.getSourceManager(), astContext.getLangOpts());

	bool cond_val;
	if(cond->EvaluateAsBooleanCondition(cond_val, astContext)) {
		if(cond->HasSideEffects(astContext)) {
			return TransformationResult{fmt::format("(({}), ({}))", cexpr, cond_val ? texpr : fexpr), ""};
		} else {
			return TransformationResult{fmt::format("({})", cond_val ? texpr : fexpr), ""};
		}
	}

	std::string var_name = util::uid(astContext, "_CExpr");

	auto const expr_type = cop.getType();
	if(expr_type->isVoidType()) {
		return TransformationResult{"((void)0)", fmt::format("if({}) {{\n{};\n}} else {{\n{};\n}}\n", cexpr, texpr, fexpr)};
	} else {
		clang::VarDecl* vd = clang::VarDecl::Create(
		  astContext, astContext.getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
		  &astContext.Idents.get(var_name), expr_type, nullptr, clang::StorageClass::SC_None);

		std::string to_hoist = fmt::format("{};\nif({}) {{\n{} = ({});\n}} else {{\n{} = ({});\n}}\n", *vd, cexpr, var_name,
		                                   texpr, var_name, fexpr);
		return TransformationResult{std::move(var_name), std::move(to_hoist)};
	}
}
