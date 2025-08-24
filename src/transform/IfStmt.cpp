#include "IfStmt.h"

#include "../check/Label.h"
#include "../check/NakedCaseOrDefault.h"
#include "../check/NullStmt.h"
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

std::optional<transform::IfStmtConfig> transform::IfStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::IfStmtConfig>(
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

	void append_else(std::string& result, transform::IfStmtConfig const& config, Context& ctx, clang::Stmt* stmt) {
		if(checks::null_stmt(stmt)) {
			return;
		}

		std::format_to(std::back_inserter(result), "else");
		if(auto ifStmt = llvm::dyn_cast<clang::IfStmt>(stmt)) {
			auto transformResult = transform::transformIfStmt(config, ctx, *ifStmt);
			if(transformResult.do_rewrite) {
				std::format_to(std::back_inserter(result), "{{\n{}\n}}", transformResult.statement);
			} else {
				append_as_compound(result, ctx, *stmt);
			}
		} else if(llvm::isa<clang::CompoundStmt>(stmt)) {
			std::format_to(std::back_inserter(result), "{}", ctx.source_text(stmt->getSourceRange()));
		} else {
			std::format_to(std::back_inserter(result), "{{\n{};\n}}", ctx.source_text(stmt->getSourceRange()));
		}
	}
} // namespace

StmtTransformResult transform::transformIfStmt(IfStmtConfig const& config, Context& ctx, clang::IfStmt& ifStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* cond = ifStmt.getCond()->IgnoreImpCasts();
	clang::Stmt* then_stmt = ifStmt.getThen();
	clang::Stmt* else_stmt = ifStmt.getElse();

	bool then_stmt_is_null_stmt = checks::null_stmt(then_stmt);
	bool else_stmt_is_null_stmt = checks::null_stmt(else_stmt);

	// If the condition is a constant, we can deconstruct the if statement.
	if(!checks::label(ifStmt) && !checks::naked_case_or_default(ifStmt)) {
		if(bool cond_val; cond->EvaluateAsBooleanCondition(cond_val, *ctx.astContext)) {
			std::string result;
			if(cond->HasSideEffects(*ctx.astContext)) {
				result = std::format("{};", ctx.source_text(cond->getSourceRange()));
			}
			if(cond_val) {
				if(!then_stmt_is_null_stmt) {
					append_as_compound(result, ctx, *then_stmt);
				}
			} else {
				if(!else_stmt_is_null_stmt) {
					append_as_compound(result, ctx, *else_stmt);
				}
			}
			if(result.empty()) {
				return {";"s};
			} else {
				return {std::move(result)};
			}
		}
	}

	// We never want to keep a null statement as the then condition
	if(then_stmt_is_null_stmt) {
		if(else_stmt_is_null_stmt) {
			return {std::format("{};", ctx.source_text(cond->getSourceRange()))};
		} else {
			auto result = std::format("if(!({}))", ctx.source_text(cond->getSourceRange()));
			append_as_compound(result, ctx, *else_stmt);
			return {std::move(result)};
		}
	}

	// Skip transformation if:
	// 1. The condition is simple enough that it need not be rewritten
	// 2. The then statement is a compound statement
	// 3. The else statement is a compound statement if it exists
	// 4. The else statement exists iff it is not a null stmt
	if(checks::isSimpleValue(*cond) && llvm::isa<clang::CompoundStmt>(then_stmt) &&
	   (!else_stmt || llvm::isa<clang::CompoundStmt>(else_stmt)) && !else_stmt == else_stmt_is_null_stmt) {
		return {};
	}

	cond = cond->IgnoreParens();
	if(checks::isSimpleValue(*cond)) {
		auto result = std::format("if({})", ctx.source_text(cond->getSourceRange()));
		append_as_compound(result, ctx, *then_stmt);
		append_else(result, config, ctx, else_stmt);
		return {std::move(result)};
	} else {
		std::string var_name = ctx.uid("IfCond");
		clang::VarDecl* vd = clang::VarDecl::Create(
		  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
		  &ctx.astContext->Idents.get(var_name), ifStmt.getCond()->getType(), nullptr, clang::StorageClass::SC_None);

		auto result = std::format("{} = ({});\nif({})", *vd, ctx.source_text(cond->getSourceRange()), var_name);
		append_as_compound(result, ctx, *then_stmt);
		append_else(result, config, ctx, else_stmt);
		return {std::move(result)};
	}
}
