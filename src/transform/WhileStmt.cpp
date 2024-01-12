#include "WhileStmt.h"

#include "../check/NakedContinue.h"
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

std::optional<transform::WhileStmtConfig> transform::WhileStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::WhileStmtConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

namespace {
	void append_as_compound(std::string& result, clang::ASTContext& astContext, clang::Stmt& stmt) {
		if(llvm::isa<clang::CompoundStmt>(stmt)) {
			fmt::format_to(std::back_inserter(result), "{}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt.getSourceRange()),
			                                           astContext.getSourceManager(), astContext.getLangOpts()));
		} else {
			fmt::format_to(std::back_inserter(result), "{{\n{};\n}}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt.getSourceRange()),
			                                           astContext.getSourceManager(), astContext.getLangOpts()));
		}
	}
} // namespace

StmtTransformResult transform::transformWhileStmt(WhileStmtConfig const& config, clang::ASTContext& astContext,
                                                  clang::WhileStmt& whileStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* cond = whileStmt.getCond()->IgnoreImpCasts();
	clang::Stmt* body = whileStmt.getBody();

	if(checks::isSimpleValue(*cond) && llvm::isa<clang::CompoundStmt>(body)) {
		return {};
	}

	cond = cond->IgnoreParens();
	if(checks::isSimpleValue(*cond)) {
		auto result = fmt::format("while({})",
		                          clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                                      astContext.getSourceManager(), astContext.getLangOpts()));
		append_as_compound(result, astContext, *body);
		return {std::move(result)};
	} else if(!checks::naked_continue(*body)) {
		std::string var_name = util::uid(astContext, "_WhileCond");
		auto cond_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                            astContext.getSourceManager(), astContext.getLangOpts());

		auto result = fmt::format("_Bool {} = ({});\nwhile({}) {{\n", var_name, cond_str, var_name);
		append_as_compound(result, astContext, *body);
		fmt::format_to(std::back_inserter(result), "{} = ({});\n}}", var_name, cond_str);
		return {std::move(result)};
	} else {
		auto cond_str = clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                            astContext.getSourceManager(), astContext.getLangOpts());

		auto result = fmt::format("while(1) {{\nif(!({})) {{\nbreak;\n}}\n", cond_str);
		append_as_compound(result, astContext, *body);
		fmt::format_to(std::back_inserter(result), "\n}}");
		return {std::move(result)};
	}
}
