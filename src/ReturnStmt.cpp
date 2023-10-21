#include "ReturnStmt.h"

#include "checks/SimpleValue.h"

#include "utils/UId.h"

#include "utils/fmtlib_clang.h"
#include "utils/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <string>

std::optional<std::string> transformReturnStmt(clang::ASTContext* astContext, clang::ReturnStmt* returnStmt) {
	clang::Expr* retVal = returnStmt->getRetValue();
	if(!retVal) {
		return {};
	}

	retVal = retVal->IgnoreImpCasts();
	if(checks::isSimpleValue(retVal)) {
		return {};
	}

	retVal = retVal->IgnoreParens();
	if(checks::isSimpleValue(retVal)) {
		return {fmt::format("return {}",
		                    clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(retVal->getSourceRange()),
		                                                astContext->getSourceManager(), astContext->getLangOpts()))};
	}

	std::string var_name = uid(astContext, "_Return");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  *astContext, astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &astContext->Idents.get(var_name), returnStmt->getRetValue()->getType(), nullptr, clang::StorageClass::SC_None);

	return {fmt::format("{} = ({});\nreturn {}", *vd,
	                    clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(retVal->getSourceRange()),
	                                                astContext->getSourceManager(), astContext->getLangOpts()),
	                    var_name)};
}
