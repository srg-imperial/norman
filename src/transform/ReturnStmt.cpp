#include "ReturnStmt.h"

#include "../check/SimpleValue.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <string>

std::optional<transform::ReturnStmtConfig> transform::ReturnStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::ReturnStmtConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

StmtTransformResult transform::transformReturnStmt(ReturnStmtConfig const& config, Context& ctx,
                                                   clang::ReturnStmt& returnStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* retVal = returnStmt.getRetValue();
	if(!retVal) {
		return {};
	}

	if(retVal->getType()->isVoidType()) {
		return {fmt::format("{};\nreturn;", ctx.source_text(retVal->getSourceRange()))};
	}

	retVal = retVal->IgnoreImpCasts();
	if(checks::isSimpleValue(*retVal)) {
		return {};
	}

	retVal = retVal->IgnoreParens();
	if(checks::isSimpleValue(*retVal)) {
		return {fmt::format("return {}", ctx.source_text(retVal->getSourceRange()))};
	}

	std::string var_name = ctx.uid("Return");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &ctx.astContext->Idents.get(var_name), returnStmt.getRetValue()->getType(), nullptr, clang::StorageClass::SC_None);

	return {fmt::format("{} = ({});\nreturn {}", *vd, ctx.source_text(retVal->getSourceRange()), var_name)};
}
