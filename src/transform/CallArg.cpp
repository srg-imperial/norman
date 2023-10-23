#include "CallArg.h"

#include "../check/SimpleValue.h"
#include "../util/UId.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <string>

std::optional<TransformationResult> transform::transformCallArg(clang::ASTContext* astContext, clang::Expr* arg) {
	clang::Expr* simplified_arg = arg->IgnoreImpCasts()->IgnoreParens();

	if(checks::isSimpleValue(simplified_arg)) {
		return {};
	}

	std::string var_name = util::uid(astContext, "_CallArg");

	clang::VarDecl* vd = clang::VarDecl::Create(
	  *astContext, astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &astContext->Idents.get(var_name), arg->getType(), nullptr, clang::StorageClass::SC_None);

	std::string to_hoist =
	  fmt::format("{} = ({});\n", *vd,
	              clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(arg->getSourceRange()),
	                                          astContext->getSourceManager(), astContext->getLangOpts()));

	return TransformationResult{std::move(var_name), std::move(to_hoist)};
}