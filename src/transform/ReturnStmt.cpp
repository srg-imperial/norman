#include "ReturnStmt.h"

#include "../check/SimpleValue.h"
#include "../util/UId.h"

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
	return BaseConfig::parse<transform::ReturnStmtConfig>(v, [](auto& config, auto const& member) { return false; });
}

std::optional<std::string> transform::transformReturnStmt(ReturnStmtConfig const& config, clang::ASTContext& astContext,
                                                          clang::ReturnStmt& returnStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* retVal = returnStmt.getRetValue();
	if(!retVal) {
		return {};
	}

	retVal = retVal->IgnoreImpCasts();
	if(checks::isSimpleValue(*retVal)) {
		return {};
	}

	retVal = retVal->IgnoreParens();
	if(checks::isSimpleValue(*retVal)) {
		return {fmt::format("return {}",
		                    clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(retVal->getSourceRange()),
		                                                astContext.getSourceManager(), astContext.getLangOpts()))};
	}

	std::string var_name = util::uid(astContext, "_Return");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  astContext, astContext.getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &astContext.Idents.get(var_name), returnStmt.getRetValue()->getType(), nullptr, clang::StorageClass::SC_None);

	return {fmt::format("{} = ({});\nreturn {}", *vd,
	                    clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(retVal->getSourceRange()),
	                                                astContext.getSourceManager(), astContext.getLangOpts()),
	                    var_name)};
}
