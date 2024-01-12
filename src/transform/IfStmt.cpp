#include "IfStmt.h"

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

std::optional<transform::IfStmtConfig> transform::IfStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::IfStmtConfig>(
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

	void append_else(std::string& result, transform::IfStmtConfig const& config, clang::ASTContext& astContext,
	                 clang::Stmt* stmt) {
		if(!stmt) {
			return;
		}

		fmt::format_to(std::back_inserter(result), "else");
		if(auto ifStmt = llvm::dyn_cast<clang::IfStmt>(stmt)) {
			auto transformResult = transform::transformIfStmt(config, astContext, *ifStmt);
			if(transformResult.do_rewrite) {
				fmt::format_to(std::back_inserter(result), "{{\n{}\n}}", transformResult.statement);
			} else {
				append_as_compound(result, astContext, *stmt);
			}
		} else if(llvm::isa<clang::CompoundStmt>(stmt)) {
			fmt::format_to(std::back_inserter(result), "{}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt->getSourceRange()),
			                                           astContext.getSourceManager(), astContext.getLangOpts()));
		} else {
			fmt::format_to(std::back_inserter(result), "{{\n{};\n}}",
			               clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(stmt->getSourceRange()),
			                                           astContext.getSourceManager(), astContext.getLangOpts()));
		}
	}
} // namespace

StmtTransformResult transform::transformIfStmt(IfStmtConfig const& config, clang::ASTContext& astContext,
                                               clang::IfStmt& ifStmt) {
	if(!config.enabled) {
		return {};
	}

	clang::Expr* cond = ifStmt.getCond()->IgnoreImpCasts();
	clang::Stmt* then_stmt = ifStmt.getThen();
	clang::Stmt* else_stmt = ifStmt.getElse();

	if(checks::isSimpleValue(*cond) && llvm::isa<clang::CompoundStmt>(then_stmt) &&
	   (!else_stmt || llvm::isa<clang::CompoundStmt>(else_stmt))) {
		return {};
	}

	cond = cond->IgnoreParens();
	if(checks::isSimpleValue(*cond)) {
		auto result =
		  fmt::format("if({})", clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                                    astContext.getSourceManager(), astContext.getLangOpts()));
		append_as_compound(result, astContext, *then_stmt);
		append_else(result, config, astContext, else_stmt);
		return {std::move(result)};
	} else {
		std::string var_name = util::uid(astContext, "_IfCond");
		clang::VarDecl* vd = clang::VarDecl::Create(
		  astContext, astContext.getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
		  &astContext.Idents.get(var_name), ifStmt.getCond()->getType(), nullptr, clang::StorageClass::SC_None);

		auto result = fmt::format("{} = ({});\nif({})", *vd,
		                          clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(cond->getSourceRange()),
		                                                      astContext.getSourceManager(), astContext.getLangOpts()),
		                          var_name);
		append_as_compound(result, astContext, *then_stmt);
		append_else(result, config, astContext, else_stmt);
		return {std::move(result)};
	}
}
