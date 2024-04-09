#include "ForStmt.h"

#include "../check/NakedContinue.h"

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
		Context& ctx;

		clang::Stmt* init;
		clang::Expr* cond;
		clang::Expr* inc;
		clang::Stmt* body;

		Out out;

	public:
		ForTransformer(Context& ctx, clang::ForStmt& forStmt, Out out)
		  : ctx{ctx}
		  , init{forStmt.getInit()}
		  , cond{forStmt.getCond()}
		  , inc{forStmt.getInc()}
		  , body{forStmt.getBody()}
		  , out{std::move(out)} { }

		void transform() {
			if(init) {
				out = fmt::format_to(out, "{{\n{};\n", ctx.source_text(init->getSourceRange()));
				transform_2();
				out = fmt::format_to(out, "}}");
			} else {
				transform_2();
			}
		}

	private:
		void transform_2() {
			if(inc) {
				auto inc_str = ctx.source_text(inc->IgnoreParens()->getSourceRange());
				if(checks::naked_continue(*body)) {
					std::string var_name = ctx.uid("_ForStmt");
					out = fmt::format_to(out, "_Bool {} = 0;\nwhile(1){{\nif({}){{\n{};\n}}\n{} = 1;\n", var_name, var_name, inc_str, var_name);
					if(cond) {
						out = fmt::format_to(out, "if(!({})) {{\nbreak;\n}}\n", ctx.source_text(cond->IgnoreParens()->getSourceRange()));
					}
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
				out = fmt::format_to(out, "while({})", ctx.source_text(cond->IgnoreParens()->getSourceRange()));
			} else {
				out = fmt::format_to(out, "while(1)");
			}
		}

		void transform_body() {
			if(llvm::isa<clang::CompoundStmt>(body)) {
				out = fmt::format_to(out, "{}", ctx.source_text(body->getSourceRange()));
			} else {
				out = fmt::format_to(out, "{{\n{}\n}}\n", ctx.source_text(body->getSourceRange()));
			}
		}
	};
} // namespace

StmtTransformResult transform::transformForStmt(ForStmtConfig const& config, Context& ctx, clang::ForStmt& forStmt) {
	if(!config.enabled) {
		return {};
	}

	std::string result;

	ForTransformer{ctx, forStmt, std::back_inserter(result)}.transform();

	return {std::move(result)};
}
