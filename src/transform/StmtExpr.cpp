#include "StmtExpr.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>

std::optional<transform::StmtExprConfig> transform::StmtExprConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::StmtExprConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

ExprTransformResult transform::transformStmtExpr(StmtExprConfig const& config, Context& ctx, clang::StmtExpr& stexpr) {
	if(!config.enabled) {
		return {};
	}

	clang::CompoundStmt* Stmts = stexpr.getSubStmt();
	clang::QualType stexpr_type = stexpr.getType();

	if(stexpr_type->isVoidType()) {
		return {"((void)0)", ctx.source_text(Stmts->getSourceRange()).str()};
	}

	std::string to_hoist;
	auto out = std::back_inserter(to_hoist);

	std::string var_name = ctx.uid("StmtExpr");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &ctx.astContext->Idents.get(var_name), stexpr_type, nullptr, clang::StorageClass::SC_None);
	out = std::format_to(out, "{};\n{{", *vd);

	for(clang::CompoundStmt::body_iterator S = Stmts->body_begin(), end = std::prev(Stmts->body_end()); S != end; ++S) {
		out = std::format_to(out, "{};\n", ctx.source_text((*S)->getSourceRange()));
	}

	clang::Stmt* finalStmt = Stmts->body_back();
	// it is impossible to jump into a statement expression (which makes naked case/default statements impossible here),
	// but the final statement may be the target of a goto from within the statement expression, for example:
	// `({goto stmt; stmt: 42;})`
	while(auto* labelStmt = llvm::dyn_cast<clang::LabelStmt>(finalStmt)) {
		out = std::format_to(out, "{}:", ctx.source_text(labelStmt->getDecl()->getSourceRange()));
		finalStmt = labelStmt->getSubStmt();
	}
	out = std::format_to(out, "{}=({});\n}}", var_name, ctx.source_text(finalStmt->getSourceRange()));
	// we need to protect against accidental token concatenation, as we now return a valid identifier where there were
	// parentheses previously
	return {std::format(" {} ", var_name), std::move(to_hoist)};
}
