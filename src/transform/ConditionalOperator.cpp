#include "ConditionalOperator.h"

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
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformConditionalOperator(ConditionalOperatorConfig const& config, Context& ctx,
                                                            clang::ConditionalOperator& cop) {
	if(!config.enabled) {
		return {};
	}

	auto const* cond = cop.getCond()->IgnoreParens();

	auto cexpr = ctx.source_text(cond->getSourceRange());
	auto texpr = ctx.source_text(cop.getTrueExpr()->IgnoreParens()->getSourceRange());
	auto fexpr = ctx.source_text(cop.getFalseExpr()->IgnoreParens()->getSourceRange());

	if(bool cond_val; cond->EvaluateAsBooleanCondition(cond_val, *ctx.astContext)) {
		if(cond->HasSideEffects(*ctx.astContext)) {
			return {fmt::format("(({}), ({}))", cexpr, cond_val ? texpr : fexpr)};
		} else {
			return {fmt::format("({})", cond_val ? texpr : fexpr)};
		}
	}

	std::string var_name = ctx.uid("CExpr");

	auto const expr_type = cop.getType();
	if(expr_type->isVoidType()) {
		return {"((void)0)", fmt::format("if({}) {{\n{};\n}} else {{\n{};\n}}\n", cexpr, texpr, fexpr)};
	} else {
		clang::VarDecl* vd = clang::VarDecl::Create(
		  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
		  &ctx.astContext->Idents.get(var_name), expr_type, nullptr, clang::StorageClass::SC_None);

		std::string to_hoist = fmt::format("{};\nif({}) {{\n{} = ({});\n}} else {{\n{} = ({});\n}}\n", *vd, cexpr, var_name,
		                                   texpr, var_name, fexpr);
		return {std::move(var_name), std::move(to_hoist)};
	}
}
