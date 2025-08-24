#include "DoStmt.h"

#include "../check/Label.h"
#include "../check/NakedBreak.h"
#include "../check/NakedCaseOrDefault.h"
#include "../check/NakedContinue.h"

#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Casting.h>

#include <iterator>
#include <string>

std::optional<transform::DoStmtConfig> transform::DoStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::DoStmtConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

namespace {
	void append_as_compound(std::string& result, Context& ctx, clang::Stmt& stmt) {
		if(llvm::isa<clang::CompoundStmt>(stmt)) {
			std::format_to(std::back_inserter(result), "{}", ctx.source_text(stmt.getSourceRange()));
		} else {
			std::format_to(std::back_inserter(result), "{{\n{};\n}}", ctx.source_text(stmt.getSourceRange()));
		}
	}
} // namespace

StmtTransformResult transform::transformDoStmt(DoStmtConfig const& config, Context& ctx, clang::DoStmt& doStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Stmt& body = *doStmt.getBody();
	clang::Expr* cond = doStmt.getCond();

	std::string result;

	if(bool cond_val; cond->EvaluateAsBooleanCondition(cond_val, *ctx.astContext)) {
		if(cond_val) {
			std::format_to(std::back_inserter(result), "while(1)");
			if(cond->HasSideEffects(*ctx.astContext)) {
				std::format_to(std::back_inserter(result), " {{");
				append_as_compound(result, ctx, body);
				std::format_to(std::back_inserter(result), "{};}}", ctx.source_text(cond->getSourceRange()));
			} else {
				append_as_compound(result, ctx, body);
			}
			return {std::move(result)};
		} else {
			if(!checks::naked_break(body) && !checks::naked_continue(body)) {
				append_as_compound(result, ctx, body);
				if(cond->HasSideEffects(*ctx.astContext)) {
					std::format_to(std::back_inserter(result), "{};", ctx.source_text(cond->getSourceRange()));
				}
				return {std::move(result)};
			}
		}
	}

	if(!checks::naked_continue(body)) {
		if(!checks::label(body) && !checks::naked_case_or_default(body) && !checks::naked_break(body)) {
			append_as_compound(result, ctx, body);
			std::format_to(std::back_inserter(result), "\nwhile({}) ", ctx.source_text(cond->getSourceRange()));
			append_as_compound(result, ctx, body);
			return {std::move(result)};
		} else {
			auto var_name = ctx.uid("DoCond");

			std::format_to(std::back_inserter(result), "_Bool {} = 1;\nwhile({}) {{\n", var_name, var_name);

			append_as_compound(result, ctx, body);

			std::format_to(std::back_inserter(result), "\n{} = ({});\n}}", var_name, ctx.source_text(cond->getSourceRange()));

			return {std::move(result)};
		}
	}

	auto var_name = ctx.uid("DoStmt");

	std::format_to(std::back_inserter(result), "_Bool {} = 1;\nwhile({} || ({})) {{\n{} = 0;\n", var_name, var_name,
	               ctx.source_text(cond->getSourceRange()), var_name);

	append_as_compound(result, ctx, body);

	std::format_to(std::back_inserter(result), "\n}}");

	return {std::move(result)};
}
