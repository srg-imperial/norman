#include "Normalise.h"

#include "check/NullStmt.h"
#include "transform/CallArg.h"
#include "transform/CommaOperator.h"
#include "transform/ConditionalOperator.h"
#include "transform/DeclStmt.h"
#include "transform/DoStmt.h"
#include "transform/ForStmt.h"
#include "transform/IfStmt.h"
#include "transform/LAndOperator.h"
#include "transform/LOrOperator.h"
#include "transform/ParenExpr.h"
#include "transform/ReturnStmt.h"
#include "transform/StmtExpr.h"
#include "transform/StringLiteral.h"
#include "transform/SwitchStmt.h"
#include "transform/UnaryExprOrTypeTraitExpr.h"
#include "transform/VarDecl.h"
#include "transform/WhileStmt.h"
#include "util/Log.h"
#include "util/frontend.h"

#include "util/fmtlib_clang.h"
#include "util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/Casting.h>

using namespace clang;

namespace {
	class NormaliseVisitor : public RecursiveASTVisitor<NormaliseVisitor> {
		std::vector<Context> ctxs;
		Config const& config;
		Config::ScopedConfig const* scopedConfig{};
		Rewriter& rewriter;
		Rewriter::RewriteOptions onlyRemoveOld;
		std::vector<std::string> to_hoist;

	public:
		explicit NormaliseVisitor(CompilerInstance* CI, Config const& config, Rewriter& rewriter)
		  : config(config)
		  , scopedConfig(&config.fileScope())
		  , rewriter(rewriter) {
			onlyRemoveOld.IncludeInsertsAtBeginOfRange = false;
			onlyRemoveOld.IncludeInsertsAtEndOfRange = false;

			ctxs.push_back(Context::FileLevel(CI->getASTContext()));
		}

		bool TraverseEmptyDecl(EmptyDecl* emptyDecl) {
			rewriter.RemoveText(emptyDecl->getSourceRange(), onlyRemoveOld);
			return true;
		}

		bool TraverseFunctionDecl(FunctionDecl* fdecl) {
			scopedConfig = &config.function(*fdecl);
			if(!scopedConfig->process) {
				logln("Skipping function ", fdecl->getName(), DisplaySourceLoc(ctxs.back().astContext, fdecl->getBeginLoc()));
				return true;
			}

			ctxs.push_back(Context::FunctionLevel(fdecl->getASTContext(), *fdecl));
			bool result = RecursiveASTVisitor::TraverseFunctionDecl(fdecl);
			ctxs.pop_back();

			scopedConfig = &config.fileScope();

			return result;
		}

		bool TraverseCallExpr(CallExpr* cexpr) {
			if(!TraverseStmt(cexpr->getCallee())) {
				return false;
			}

			for(unsigned i = 0, num_args = cexpr->getNumArgs(); i < num_args; i++) {
				Expr* arg = cexpr->getArg(i);

				auto transformResult = transform::transformCallArg(scopedConfig->configCallArg, ctxs.back(), *arg);
				if(transformResult.do_rewrite) {
					if(!transformResult.to_hoist.empty()) {
						to_hoist.emplace_back(std::move(transformResult.to_hoist));
					}
					rewriter.RemoveText(arg->getSourceRange(), onlyRemoveOld);
					rewriter.InsertTextAfter(arg->getSourceRange().getBegin(), transformResult.expression);

					logln("Transformation applied at: ", DisplaySourceLoc(ctxs.back().astContext, arg->getBeginLoc()));
				} else {
					if(!RecursiveASTVisitor<NormaliseVisitor>::TraverseStmt(arg)) {
						return false;
					}
				}
			}

			return true;
		}

		bool TraverseBinaryOperator(BinaryOperator* binop) {
			switch(binop->getOpcode()) {
				case clang::BinaryOperator::Opcode::BO_Comma: return TraverseCommaOperator(binop);
				case clang::BinaryOperator::Opcode::BO_LAnd: return TraverseLAndOperator(binop);
				case clang::BinaryOperator::Opcode::BO_LOr: return TraverseLOrOperator(binop);
				default: return RecursiveASTVisitor<NormaliseVisitor>::TraverseBinaryOperator(binop);
			}
		}

#define TraverseExprFn(kind, type)                                                                                     \
	bool Traverse##kind(type* node) {                                                                                    \
		logFnScope("for source line ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));                     \
                                                                                                                       \
		auto transformResult = transform::transform##kind(scopedConfig->config##kind, ctxs.back(), *node);                 \
		if(transformResult.do_rewrite) {                                                                                   \
			if(!transformResult.to_hoist.empty()) {                                                                          \
				to_hoist.emplace_back(std::move(transformResult.to_hoist));                                                    \
			}                                                                                                                \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), transformResult.expression);                         \
                                                                                                                       \
			logln("Transformation applied from ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()), " to ",     \
			      DisplaySourceLoc(ctxs.back().astContext, node->getEndLoc()));                                              \
			return true;                                                                                                     \
		}                                                                                                                  \
                                                                                                                       \
		return RecursiveASTVisitor<NormaliseVisitor>::Traverse##type(node);                                                \
	}

#define TraverseStmtFn(type)                                                                                           \
	bool Traverse##type(type* node) {                                                                                    \
		logFnScope("for source line ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));                     \
                                                                                                                       \
		auto transformResult = transform::transform##type(scopedConfig->config##type, ctxs.back(), *node);                 \
		if(transformResult.do_rewrite) {                                                                                   \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), transformResult.statement);                          \
                                                                                                                       \
			logln("Transformation applied from ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()), " to ",     \
			      DisplaySourceLoc(ctxs.back().astContext, node->getEndLoc()));                                              \
			return true;                                                                                                     \
		}                                                                                                                  \
                                                                                                                       \
		return RecursiveASTVisitor<NormaliseVisitor>::Traverse##type(node);                                                \
	}

		TraverseExprFn(CommaOperator, BinaryOperator);
		TraverseExprFn(ConditionalOperator, ConditionalOperator);
		TraverseExprFn(LAndOperator, BinaryOperator);
		TraverseExprFn(LOrOperator, BinaryOperator);
		TraverseExprFn(ParenExpr, ParenExpr);
		TraverseExprFn(StmtExpr, StmtExpr);
		TraverseExprFn(StringLiteral, StringLiteral);
		TraverseExprFn(UnaryExprOrTypeTraitExpr, UnaryExprOrTypeTraitExpr);

		TraverseStmtFn(DeclStmt);
		TraverseStmtFn(DoStmt);
		TraverseStmtFn(ForStmt);
		TraverseStmtFn(IfStmt);
		TraverseStmtFn(ReturnStmt);
		TraverseStmtFn(SwitchStmt);
		TraverseStmtFn(VarDecl);
		TraverseStmtFn(WhileStmt);

		bool TraverseCompoundStmt(CompoundStmt* cstmt) {
			if(!cstmt->body_empty() && std::next(cstmt->body().begin()) == cstmt->body().end()) {
				auto* stmt = *cstmt->body().begin();
				if(auto* ccstmt = llvm::dyn_cast<CompoundStmt>(stmt)) {
					rewriter.RemoveText(ccstmt->getLBracLoc(), onlyRemoveOld);
					rewriter.RemoveText(ccstmt->getRBracLoc(), onlyRemoveOld);
					logln("Nested block removed at: ", DisplaySourceLoc(ctxs.back().astContext, ccstmt->getBeginLoc()));

					return true;
				}
			}

			for(CompoundStmt::body_iterator iter = cstmt->body_begin(), end = cstmt->body_end(); iter != end; ++iter) {
				clang::Stmt* stmt = *iter;
				assert(to_hoist.empty());

				// In general, we cannot remove labeled or attributed statements.
				// E.g., `switch(x) { default:; }` would become `switch(x) { default: }`, which causes a syntax error
				if(checks::null_stmt(stmt)) {
					rewriter.RemoveText(stmt->getSourceRange(), onlyRemoveOld);
					logln("Null stmt removed at: ", DisplaySourceLoc(ctxs.back().astContext, stmt->getBeginLoc()));
					continue;
				}

				// All other transforms shall not completely remove a statement (they can still convert it to a null statement),
				// which enables us to ignore any labels here.
				bool rewritten = false;
				while(auto labelStmt = dyn_cast<LabelStmt>(stmt)) {
					auto labelDecl = labelStmt->getDecl();
					if(ctxs.back().usedLabels.count(labelDecl)) {
						stmt = labelStmt->getSubStmt();
					} else {
						rewriter.RemoveText(labelStmt->getSourceRange(), onlyRemoveOld);
						rewriter.InsertTextBefore(labelStmt->getBeginLoc(),
						                          ctxs.back().source_text(labelStmt->getSubStmt()->getSourceRange()));
						rewritten = true;
						break;
					}
				}
				if(rewritten) {
					continue;
				}

				while(auto parenExpr = dyn_cast<ParenExpr>(stmt)) {
					rewriter.RemoveText(parenExpr->getLParen(), onlyRemoveOld);
					rewriter.RemoveText(parenExpr->getRParen(), onlyRemoveOld);
					logln("Parenthesis removed at: ", DisplaySourceLoc(ctxs.back().astContext, parenExpr->getBeginLoc()));

					stmt = parenExpr->getSubExpr();
				}
				if(!TraverseStmt(stmt)) {
					return false;
				}
				for(std::size_t i = to_hoist.size(); i > 0;) {
					--i;
					rewriter.InsertTextBefore(stmt->getBeginLoc(), to_hoist[i]);
				}
				if(!to_hoist.empty() && stmt != *iter) {
					// Some `clang::Stmt`s (e.g., `int x;`) are not actually "statements" in the sense that they are valid targets
					// for labels and similar constructs.
					// To make it a bit easier to use, we insert a null statement to serve as a target instead.
					rewriter.InsertTextBefore(stmt->getBeginLoc(), ";\n");
				}
				to_hoist.clear();
			}
			return true;
		}

		bool TraverseTypeLoc([[maybe_unused]] TypeLoc typeLoc) {
			// Let's not traverse into types.
			//
			// Known issues with traversing into types:
			// - `typeof(X)` expressions are parsed as (typeof (paren (X))), which means we end up potentially stripping an
			// "unnecessary" paren expr, resulting in `typeof X`, which does not compile
			return true;
		}
	};
} // namespace

std::unique_ptr<clang::tooling::FrontendActionFactory> newNormaliseFrontendFactory(bool& rewritten, Config& config) {
	using Action = ClangRewriterFrontendAction<SimpleASTConsumer<NormaliseVisitor>>;
	return std::make_unique<FrontendActionDataFactory<Action, bool*, Config*>>(&rewritten, &config);
}