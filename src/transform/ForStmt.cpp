#include "ForStmt.h"

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

std::optional<transform::ForStmtConfig> transform::ForStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::ForStmtConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

namespace {
	template <typename Out> class ForTransformer {
		clang::ASTContext* astContext;

		clang::Stmt* init;
		clang::Expr* cond;
		clang::Expr* inc;
		clang::Stmt* body;

		Out out;

	public:
		ForTransformer(clang::ASTContext& astContext, clang::ForStmt& forStmt, Out out)
		  : astContext{&astContext}
		  , init{forStmt.getInit()}
		  , cond{forStmt.getCond()}
		  , inc{forStmt.getInc()}
		  , body{forStmt.getBody()}
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
				if(checks::naked_continue(*body)) {
					std::string var_name = util::uid(*astContext, "_ForStmt");
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
			if(llvm::isa<clang::CompoundStmt>(body)) {
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

StmtTransformResult transform::transformForStmt(ForStmtConfig const& config, clang::ASTContext& astContext,
                                                clang::ForStmt& forStmt) {
	if(!config.enabled) {
		return {};
	}

	std::string result;

	ForTransformer{astContext, forStmt, std::back_inserter(result)}.transform();

	return {std::move(result)};
}
