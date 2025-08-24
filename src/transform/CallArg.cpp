#include "CallArg.h"

#include "../check/SimpleValue.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <string>

std::optional<transform::CallArgConfig> transform::CallArgConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::CallArgConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformCallArg(CallArgConfig const& config, Context& ctx, clang::Expr& arg) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* simplified_arg = arg.IgnoreImpCasts()->IgnoreParens();

	if(checks::isSimpleValue(*simplified_arg)) {
		return {};
	}

	std::string var_name = ctx.uid("CallArg");

	clang::VarDecl* vd = clang::VarDecl::Create(
	  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &ctx.astContext->Idents.get(var_name), arg.getType(), nullptr, clang::StorageClass::SC_None);

	std::string to_hoist = std::format("{} = ({});\n", *vd, ctx.source_text(arg.getSourceRange()));

	return {std::move(var_name), std::move(to_hoist)};
}
