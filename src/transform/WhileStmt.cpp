#include "WhileStmt.h"

#include "../check/Label.h"
#include "../check/NakedCaseOrDefault.h"
#include "../check/NakedContinue.h"
#include "../check/SimpleValue.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/raw_ostream.h>

#include <iterator>
#include <string>

using namespace std::literals;

std::optional<transform::WhileStmtConfig> transform::WhileStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::WhileStmtConfig>(
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

StmtTransformResult transform::transformWhileStmt(WhileStmtConfig const& config, Context& ctx,
                                                  clang::WhileStmt& whileStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* cond = whileStmt.getCond()->IgnoreImpCasts();
	clang::Stmt* body = whileStmt.getBody();

	// If the condition is a constant false, we can deconstruct the while statement.
	if(!checks::label(*body) && !checks::naked_case_or_default(*body)) {
		if(bool cond_val; cond->EvaluateAsBooleanCondition(cond_val, *ctx.astContext)) {
			if(!cond_val) {
				if(cond->HasSideEffects(*ctx.astContext) || checks::label(*cond) || checks::naked_case_or_default(*cond)) {
					return {std::format("{};", ctx.source_text(cond->getSourceRange()))};
				} else {
					return {";"s};
				}
			}
		}
	}

	if(checks::isSimpleValue(*cond) && llvm::isa<clang::CompoundStmt>(body)) {
		return {};
	}

	cond = cond->IgnoreParens();
	if(checks::isSimpleValue(*cond)) {
		auto result = std::format("while({})", ctx.source_text(cond->getSourceRange()));
		append_as_compound(result, ctx, *body);
		return {std::move(result)};
	} else if(!checks::naked_continue(*body)) {
		std::string var_name = ctx.uid("WhileCond");
		auto cond_str = ctx.source_text(cond->getSourceRange());

		auto result = std::format("_Bool {} = ({});\nwhile({}) {{\n", var_name, cond_str, var_name);
		append_as_compound(result, ctx, *body);
		std::format_to(std::back_inserter(result), "{} = ({});\n}}", var_name, cond_str);
		return {std::move(result)};
	} else {
		auto cond_str = ctx.source_text(cond->getSourceRange());

		auto result = std::format("while(1) {{\nif(!({})) {{\nbreak;\n}}\n", cond_str);
		append_as_compound(result, ctx, *body);
		std::format_to(std::back_inserter(result), "\n}}");
		return {std::move(result)};
	}
}
