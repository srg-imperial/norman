#include "Config.h"
#include "check/NullStmt.h"
#include "transform/CallArg.h"
#include "transform/CommaOperator.h"
#include "transform/ConditionalOperator.h"
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
#include "transform/VarDecl.h"
#include "transform/WhileStmt.h"
#include "util/Log.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Driver/Options.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

using namespace clang;

class NormaliseExprVisitor : public RecursiveASTVisitor<NormaliseExprVisitor> {
	std::vector<Context> ctxs;
	Config const& config;
	Config::ScopedConfig const* scopedConfig{};
	Rewriter& rewriter;
	Rewriter::RewriteOptions onlyRemoveOld;
	std::vector<std::string> to_hoist;

public:
	explicit NormaliseExprVisitor(CompilerInstance* CI, Config const& config, Rewriter& rewriter)
	  : config(config)
	  , scopedConfig(&config.fileScope())
	  , rewriter(rewriter) {
		onlyRemoveOld.IncludeInsertsAtBeginOfRange = false;
		onlyRemoveOld.IncludeInsertsAtEndOfRange = false;

		ctxs.push_back(Context{&CI->getASTContext(), std::string{}});
	}

	bool TraverseFunctionDecl(FunctionDecl* fdecl) {
		scopedConfig = &config.function(*fdecl);
		if(!scopedConfig->process) {
			logln("Skipping function ", fdecl->getName(), DisplaySourceLoc(ctxs.back().astContext, fdecl->getBeginLoc()));
			return true;
		}

		ctxs.push_back(Context{&fdecl->getASTContext(), fdecl->getName().str()});
		bool result = RecursiveASTVisitor::TraverseFunctionDecl(fdecl);
		ctxs.pop_back();

		scopedConfig = &config.fileScope();

		return result;
	}

	bool TraverseEnumDecl(EnumDecl*) { return true; }

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
				if(!RecursiveASTVisitor<NormaliseExprVisitor>::TraverseStmt(arg)) {
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
			default: return RecursiveASTVisitor<NormaliseExprVisitor>::TraverseBinaryOperator(binop);
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
			logln("Transformation applied at: ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));             \
			return true;                                                                                                     \
		}                                                                                                                  \
                                                                                                                       \
		return RecursiveASTVisitor<NormaliseExprVisitor>::Traverse##type(node);                                            \
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
			logln("Transformation applied at: ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));             \
			return true;                                                                                                     \
		}                                                                                                                  \
                                                                                                                       \
		return RecursiveASTVisitor<NormaliseExprVisitor>::Traverse##type(node);                                            \
	}

	TraverseExprFn(CommaOperator, BinaryOperator);
	TraverseExprFn(ConditionalOperator, ConditionalOperator);
	TraverseExprFn(LAndOperator, BinaryOperator);
	TraverseExprFn(LOrOperator, BinaryOperator);
	TraverseExprFn(ParenExpr, ParenExpr);
	TraverseExprFn(StmtExpr, StmtExpr);
	TraverseExprFn(StringLiteral, StringLiteral);

	TraverseStmtFn(DoStmt);
	TraverseStmtFn(ForStmt);
	TraverseStmtFn(IfStmt);
	TraverseStmtFn(ReturnStmt);
	TraverseStmtFn(SwitchStmt);
	TraverseStmtFn(VarDecl);
	TraverseStmtFn(WhileStmt);

	bool TraverseCompoundStmt(CompoundStmt* cstmt) {
		if (!cstmt->body().empty() && std::next(cstmt->body().begin()) == cstmt->body().end()) {
			auto* stmt = *cstmt->body().begin();
			if (auto* ccstmt = llvm::dyn_cast<CompoundStmt>(stmt)) {
				rewriter.RemoveText(ccstmt->getLBracLoc(), onlyRemoveOld);
				rewriter.RemoveText(ccstmt->getRBracLoc(), onlyRemoveOld);
				logln("Nested block removed at: ", DisplaySourceLoc(ctxs.back().astContext, ccstmt->getBeginLoc()));
			
				return true;
			}
		}

		for(auto* stmt : cstmt->body()) {
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
			while(auto labelStmt = dyn_cast<LabelStmt>(stmt)) {
				stmt = labelStmt->getSubStmt();
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

class NormaliseExprASTConsumer final : public ASTConsumer {
	NormaliseExprVisitor visitor;

public:
	explicit NormaliseExprASTConsumer(CompilerInstance* CI, Config const& config, Rewriter& rewriter)
	  : visitor{CI, config, rewriter} { }

	void HandleTranslationUnit(ASTContext& Context) { visitor.TraverseDecl(Context.getTranslationUnitDecl()); }
};

class NormaliseExprFrontendAction final : public ASTFrontendAction {
	Config const& config;
	Rewriter rewriter;
	bool& rewritten;

public:
	NormaliseExprFrontendAction(bool* rewritten, Config const* config)
	  : config{*config}
	  , rewritten{*rewritten} { }

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) override {
		return std::make_unique<NormaliseExprASTConsumer>(&CI, config, rewriter);
	}

	bool PrepareToExecuteAction(CompilerInstance& CI) override {
		rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return ASTFrontendAction::PrepareToExecuteAction(CI);
	}

	void EndSourceFileAction() override {
		if(RewriteBuffer const* rb = rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID())) {
			auto name = rewriter.getSourceMgr().getFileEntryForID(rewriter.getSourceMgr().getMainFileID())->getName();

			std::error_code error_code;
			llvm::raw_fd_ostream outFile{name, error_code};
			if(error_code) {
				llvm::errs() << error_code.message() << "\n";
				std::exit(1);
			}
			rb->write(outFile);
			outFile.close();
			rewritten = true;
		} else {
			rewritten = false;
		}
	}
};

template <typename T, typename... V> class FrontendActionDataFactory : public clang::tooling::FrontendActionFactory {
	std::tuple<typename std::decay<V>::type...> args;

	template <std::size_t... I> std::unique_ptr<FrontendAction> create_impl(std::index_sequence<I...>) {
		return std::make_unique<T>(std::get<I>(args)...);
	}

public:
	FrontendActionDataFactory(V&&... args)
	  : args(std::forward<V>(args)...) { }

	std::unique_ptr<FrontendAction> create() override { return create_impl(std::index_sequence_for<V...>{}); }
};

template <typename T, typename... V>
std::unique_ptr<clang::tooling::FrontendActionFactory> newFrontendActionDataFactory(V&&... args) {
	return std::make_unique<FrontendActionDataFactory<T, V...>>(std::forward<V>(args)...);
}

int main(int argc, const char** argv) {
	llvm::InitLLVM x{argc, argv};

	static llvm::cl::OptionCategory NormanOptionCategory("Norman's Options");

	static llvm::cl::opt<std::string> configPath(
	  "config", llvm::cl::desc("Path to the file containing a function config"), llvm::cl::cat(NormanOptionCategory));

	llvm::cl::HideUnrelatedOptions(NormanOptionCategory);

#if LLVM_VERSION_MAJOR > 12
	auto op = clang::tooling::CommonOptionsParser::create(argc, argv, NormanOptionCategory);
	if(!op) {
		llvm::errs() << "Could not create options parser!\n";
		return EXIT_FAILURE;
	}
#else
	std::optional<clang::tooling::CommonOptionsParser> op{{argc, argv, NormanOptionCategory}};
#endif

	Config config;
	if(auto const& path = configPath.getValue(); !path.empty()) {
		if(auto parsed = Config::from_file(path)) {
			config = std::move(*parsed);
		} else {
			llvm::errs() << "Could not open config file!\n";
			return EXIT_FAILURE;
		}
	}

	bool rewritten;
	std::uint64_t count = 0;
	do {
		clang::tooling::ClangTool Tool{op->getCompilations(), op->getSourcePathList()[0]};
		// disable printing warnings
		Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-w"));

		int result = Tool.run(&(*newFrontendActionDataFactory<NormaliseExprFrontendAction>(&rewritten, &config)));
		if(result != EXIT_SUCCESS) {
			return result;
		}
		++count;
		logln(rewritten ? "File was rewritten" : "No change");
	} while(rewritten);
	llvm::outs() << "Norman ran " << count << " times to normalize your source code.\n";
}
