#include "ForStmt.h"

#include "checks/NakedContinue.h"

#include "utils/UId.h"

#include "utils/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

#include <iterator>
#include <string>

using llvm::isa;

namespace {
	template <typename Out> class ForTransformer {
		clang::ASTContext* astContext;

		clang::Stmt* init;
		clang::Expr* cond;
		clang::Expr* inc;
		clang::Stmt* body;

		Out out;

	public:
		ForTransformer(clang::ASTContext* astContext, clang::ForStmt* forStmt, Out out)
		  : astContext{astContext}
		  , init{forStmt->getInit()}
		  , cond{forStmt->getCond()}
		  , inc{forStmt->getInc()}
		  , body{forStmt->getBody()}
		  , out{std::move(out)} { }

		void transform() {
			if(init) {
				out = fmt::format_to(out, "{{\n{};\n",
				                     clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(init->getSourceRange()),
				                                                 astContext->getSourceManager(), astContext->getLangOpts()));
				transform_2();
				out = fmt::format_to(out, "}}");
			} else {
				transform_2();
			}
		}

	private:
		void transform_2() {
			if(inc) {
				auto inc_str =
				  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(inc->IgnoreParens()->getSourceRange()),
				                              astContext->getSourceManager(), astContext->getLangOpts());
				if(checks::naked_continue(body)) {
					std::string var_name = uid(astContext, "_ForStmt");
					out = fmt::format_to(out, "_Bool {} = 0;", var_name);
					transform_cond();
					out = fmt::format_to(out, "{{\nif({}){{\n{};\n}}\n{} = 1;\n", var_name, inc_str, var_name);
					transform_body();
					out = fmt::format_to(out, "\n}}");
				} else {
					transform_cond();
					out = fmt::format_to(out, "{{\n");
					transform_body();
					out = fmt::format_to(out, "{};\n}}", inc_str);
				}
			} else {
				transform_cond();
				transform_body();
			}
		}

		void transform_cond() {
			if(cond) {
				out = fmt::format_to(
				  out, "while({})",
				  clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->IgnoreParens()->getSourceRange()),
				                              astContext->getSourceManager(), astContext->getLangOpts()));
			} else {
				out = fmt::format_to(out, "while(1)");
			}
		}

		void transform_body() {
			if(isa<clang::CompoundStmt>(body)) {
				out = fmt::format_to(out, "{}",
				                     clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(body->getSourceRange()),
				                                                 astContext->getSourceManager(), astContext->getLangOpts()));
			} else {
				out = fmt::format_to(out, "{{\n{}\n}}\n",
				                     clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(body->getSourceRange()),
				                                                 astContext->getSourceManager(), astContext->getLangOpts()));
			}
		}
	};
} // namespace

std::optional<std::string> transformForStmt(clang::ASTContext* astContext, clang::ForStmt* forStmt) {
	std::string result;

	ForTransformer{astContext, forStmt, std::back_inserter(result)}.transform();

	return {std::move(result)};
}
