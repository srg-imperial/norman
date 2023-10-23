#include "DoStmt.h"

#include "../check/Label.h"
#include "../check/NakedBreak.h"
#include "../check/NakedContinue.h"
#include "../util/UId.h"

#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

#include <iterator>
#include <string>

using llvm::isa;

namespace {
	void append_as_compound(std::string& result, clang::ASTContext* astContext, clang::Stmt* stmt) {
		if(isa<clang::CompoundStmt>(stmt)) {
			fmt::format_to(std::back_inserter(result), "{}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt->getSourceRange()),
			                                           astContext->getSourceManager(), astContext->getLangOpts()));
		} else {
			fmt::format_to(std::back_inserter(result), "{{\n{};\n}}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt->getSourceRange()),
			                                           astContext->getSourceManager(), astContext->getLangOpts()));
		}
	}
} // namespace

std::optional<std::string> transform::transformDoStmt(clang::ASTContext* astContext, clang::DoStmt* doStmt) {
	clang::Stmt* body = doStmt->getBody();
	clang::Expr* cond = doStmt->getCond();

	std::string result;

	if(bool cond_val; cond->EvaluateAsBooleanCondition(cond_val, *astContext)) {
		if(cond_val) {
			fmt::format_to(std::back_inserter(result), "while(1)");
			if(cond->HasSideEffects(*astContext)) {
				fmt::format_to(std::back_inserter(result), " {{");
				append_as_compound(result, astContext, body);
				fmt::format_to(std::back_inserter(result), "{};}}",
				               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
				                                           astContext->getSourceManager(), astContext->getLangOpts()));
			} else {
				append_as_compound(result, astContext, body);
			}
			return {std::move(result)};
		} else {
			if(!checks::naked_break(body) && !checks::naked_continue(body)) {
				append_as_compound(result, astContext, body);
				if(cond->HasSideEffects(*astContext)) {
					fmt::format_to(std::back_inserter(result), "{};",
					               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
					                                           astContext->getSourceManager(), astContext->getLangOpts()));
				}
				return {std::move(result)};
			}
		}
	}

	if(!checks::label(body)) {
		append_as_compound(result, astContext, body);
		fmt::format_to(std::back_inserter(result), "\nwhile({}) ",
		               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                           astContext->getSourceManager(), astContext->getLangOpts()));
		append_as_compound(result, astContext, body);
		return {std::move(result)};
	}

	auto var_name = util::uid(astContext, "_DoStmt");

	fmt::format_to(std::back_inserter(result), "_Bool {} = 1;\nwhile({} || ({})) {{\n{} = 0;\n", var_name, var_name,
	               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
	                                           astContext->getSourceManager(), astContext->getLangOpts()),
	               var_name);

	append_as_compound(result, astContext, body);

	fmt::format_to(std::back_inserter(result), "\n}}");

	return {std::move(result)};
}
