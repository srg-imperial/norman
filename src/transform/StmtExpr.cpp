#include "StmtExpr.h"

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

std::optional<transform::StmtExprConfig> transform::StmtExprConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::StmtExprConfig>(v, [](auto& config, auto const& member) { return false; });
}

ExprTransformResult transform::transformStmtExpr(StmtExprConfig const& config, clang::ASTContext& astContext,
                                                 clang::StmtExpr& stexpr) {
	if(!config.enabled) {
		return {};
	}

	clang::CompoundStmt* Stmts = stexpr.getSubStmt();
	clang::QualType stexpr_type = stexpr.getType();

	if(stexpr_type->isVoidType()) {
		return {"((void)0)", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(Stmts->getSourceRange()),
		                                                 astContext.getSourceManager(), astContext.getLangOpts())
		                       .str()};
	}

	std::string to_hoist;
	auto out = std::back_inserter(to_hoist);

	std::string var_name = util::uid(astContext, "_StmtExpr");
	clang::VarDecl* vd = clang::VarDecl::Create(astContext, astContext.getTranslationUnitDecl(), clang::SourceLocation(),
	                                            clang::SourceLocation(), &astContext.Idents.get(var_name), stexpr_type,
	                                            nullptr, clang::StorageClass::SC_None);
	out = fmt::format_to(out, "{};\n{{", *vd);

	for(clang::CompoundStmt::body_iterator S = Stmts->body_begin(), end = std::prev(Stmts->body_end()); S != end; ++S) {
		out = fmt::format_to(out, "{};\n",
		                     clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange((*S)->getSourceRange()),
		                                                 astContext.getSourceManager(), astContext.getLangOpts()));
	}

	out = fmt::format_to(
	  out, "{}=({});\n}}", var_name,
	  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(Stmts->body_back()->getSourceRange()),
	                              astContext.getSourceManager(), astContext.getLangOpts()));
	return {std::move(var_name), std::move(to_hoist)};
}
