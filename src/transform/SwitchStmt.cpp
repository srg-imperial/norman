#include "SwitchStmt.h"

#include "../check/Label.h"
#include "../check/NakedBreak.h"
#include "../check/NakedCaseOrDefault.h"
#include "../check/NakedContinue.h"

#include "../util/fmtlib_clang.h"
#include "../util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

#include <cassert>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

std::optional<transform::SwitchStmtConfig> transform::SwitchStmtConfig::parse(rapidjson::Value const& v) {
	return BaseConfig::parse<transform::SwitchStmtConfig>(
	  v, []([[maybe_unused]] auto& config, [[maybe_unused]] auto const& member) { return false; });
}

namespace {
	void printStmts(std::string& result, Context& ctx, std::vector<std::pair<clang::Stmt*, bool>> const& stmts,
	                std::size_t startIndex) {
		bool hasBreak = false;
		std::size_t end = stmts.size();
		for(std::size_t j = startIndex; j < stmts.size(); ++j) {
			if(llvm::isa<clang::BreakStmt>(stmts[j].first)) {
				// since there are no labels in this `switch`, an unconditional `break` cannot be passed
				end = j;
				break;
			}
			if(stmts[j].second) {
				hasBreak = true;
				break;
			}
		}

		if(hasBreak) {
			fmt::format_to(std::back_inserter(result), "{{\ndo {{\n");
		} else {
			fmt::format_to(std::back_inserter(result), "{{\n");
		}

		for(std::size_t j = startIndex; j < end; ++j) {
			// this may introduce null statements if the current statement does not require a semicolon to terminate it. For
			// example `if(1) {}` would be printed as `if(1) {};`
			fmt::format_to(std::back_inserter(result), "{};", ctx.source_text(stmts[j].first->getSourceRange()));
		}

		if(hasBreak) {
			fmt::format_to(std::back_inserter(result), "}} while(0);\n}}");
		} else {
			fmt::format_to(std::back_inserter(result), "}}");
		}
	}
} // namespace

StmtTransformResult transform::transformSwitchStmt(SwitchStmtConfig const& config, Context& ctx,
                                                   clang::SwitchStmt& switchStmt) {
	if(!config.enabled) {
		return {};
	}

	// naked continue statements clash with our rewrite, as they are now caught by the `switch' replacement instead of a
	// surrounding loop
	if(checks::naked_continue(switchStmt)) {
		throw "naked continue found";
	}

	// label statements are an issue, as we duplicate statements - which would duplicate the labels
	if(checks::label(switchStmt)) {
		throw "label found";
	}

	clang::Expr* cond = switchStmt.getCond();

	auto var_name = ctx.uid("_Switch");
	clang::VarDecl* vd = clang::VarDecl::Create(
	  *ctx.astContext, ctx.astContext->getTranslationUnitDecl(), clang::SourceLocation(), clang::SourceLocation(),
	  &ctx.astContext->Idents.get(var_name), cond->getType(), nullptr, clang::StorageClass::SC_None);
	std::string result = fmt::format("{} = ({});\n", *vd, ctx.source_text(cond->getSourceRange()));

	if(clang::CompoundStmt* body = llvm::dyn_cast<clang::CompoundStmt>(switchStmt.getBody())) {
		std::optional<std::size_t> defaultStmtTarget;
		std::vector<std::pair<std::vector<clang::CaseStmt*>, std::size_t>> caseStmts;
		std::vector<std::pair<clang::Stmt*, bool>> stmts;

		for(clang::Stmt* stmt : body->body()) {
			for(;;) {
				if(clang::CaseStmt* caseStmt = llvm::dyn_cast<clang::CaseStmt>(stmt)) {
					if(defaultStmtTarget && *defaultStmtTarget == stmts.size()) {
						// this case targets the same stmt as the default statement
					} else if(caseStmts.size() && caseStmts.back().second == stmts.size()) {
						caseStmts.back().first.push_back(caseStmt);
					} else {
						caseStmts.emplace_back(std::vector{caseStmt}, stmts.size());
					}
					stmt = caseStmt->getSubStmt();
				} else if(clang::DefaultStmt* defaultStmt = llvm::dyn_cast<clang::DefaultStmt>(stmt)) {
					if(defaultStmtTarget) {
						throw "duplicate default statement";
					}
					if(caseStmts.size() && caseStmts.back().second == stmts.size()) {
						caseStmts.pop_back();
					}
					defaultStmtTarget.emplace(stmts.size());
					stmt = defaultStmt->getSubStmt();
				} else {
					break;
				}
			}

			if(checks::naked_case_or_default(*stmt)) {
				throw "nested case or default statement";
			}

			stmts.emplace_back(stmt, checks::naked_break(*stmt));
		}

		for(std::size_t i = 0; i < caseStmts.size(); ++i) {
			if(i > 0) {
				fmt::format_to(std::back_inserter(result), "else ");
			}
			fmt::format_to(std::back_inserter(result), "if (");

			for(std::size_t j = 0; j < caseStmts[i].first.size(); ++j) {
				clang::CaseStmt* caseStmt = caseStmts[i].first[j];

				if(j > 0) {
					fmt::format_to(std::back_inserter(result), " || ");
				}

				clang::Expr* lhs = caseStmt->getLHS();
				if(clang::Expr* rhs = caseStmt->getRHS()) {
					if(caseStmts[i].first.size() != 1) {
						fmt::format_to(std::back_inserter(result), "(");
					}
					fmt::format_to(std::back_inserter(result), "({}) <= {} && {} <= ({]})",
					               ctx.source_text(lhs->getSourceRange()), var_name, var_name,
					               ctx.source_text(rhs->getSourceRange()));
					if(caseStmts[i].first.size() != 1) {
						fmt::format_to(std::back_inserter(result), ")");
					}
				} else {
					fmt::format_to(std::back_inserter(result), "{} == ({})", var_name, ctx.source_text(lhs->getSourceRange()));
				}
			}

			fmt::format_to(std::back_inserter(result), ") ");
			printStmts(result, ctx, stmts, caseStmts[i].second);
		}

		if(defaultStmtTarget) {
			fmt::format_to(std::back_inserter(result), " else ");
			printStmts(result, ctx, stmts, *defaultStmtTarget);
		}

		return {std::move(result)};
	} else {
		return {fmt::format("switch({}) {{\n{};\n}}", ctx.source_text(cond->getSourceRange()),
		                    ctx.source_text(switchStmt.getBody()->getSourceRange()))};
	}
}
