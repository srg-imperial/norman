#include "Config.h"
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

#include "util/fmtlib_clang.h"
#include "util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

using namespace clang;

class TUFixupVisitor : public RecursiveASTVisitor<TUFixupVisitor> {
	Rewriter& rewriter;
	Rewriter::RewriteOptions onlyRemoveOld;

public:
	explicit TUFixupVisitor(CompilerInstance*, Config const&, Rewriter& rewriter)
	  : rewriter(rewriter) {
		onlyRemoveOld.IncludeInsertsAtBeginOfRange = false;
		onlyRemoveOld.IncludeInsertsAtEndOfRange = false;
	}

	void reinsertDecl(Context const& ctx, Decl* decl) {
		auto text = ctx.source_text(decl->getSourceRange());
		rewriter.RemoveText(decl->getSourceRange(), onlyRemoveOld);
		rewriter.InsertTextAfter(decl->getBeginLoc(), text);
		rewriter.InsertTextAfter(decl->getBeginLoc(), ";");
	}

	bool TraverseTranslationUnitDecl(TranslationUnitDecl* tuDecl) {
		auto ctx = Context::FileLevel(tuDecl->getASTContext());

		for(auto* decl : tuDecl->decls()) {
			if(decl->isImplicit()) {
				// do nothing
			} else if(auto* recordDecl = llvm::dyn_cast<RecordDecl>(decl)) {
				if(recordDecl->getName().empty()) {
					auto id = &ctx.astContext->Idents.get(ctx.uid("_Decl"));
					recordDecl->setDeclName(clang::DeclarationName{id});
					rewriter.RemoveText(recordDecl->getSourceRange(), onlyRemoveOld);
					rewriter.InsertTextAfter(recordDecl->getBeginLoc(), fmt::format("{};", *recordDecl));
				} else {
					reinsertDecl(ctx, decl);
				}
			} else if(auto* enumDecl = llvm::dyn_cast<EnumDecl>(decl)) {
				if(enumDecl->getName().empty()) {
					auto id = &ctx.astContext->Idents.get(ctx.uid("_Decl"));
					enumDecl->setDeclName(clang::DeclarationName{id});
					rewriter.RemoveText(enumDecl->getSourceRange(), onlyRemoveOld);
					rewriter.InsertTextAfter(enumDecl->getBeginLoc(), fmt::format("{};", *enumDecl));
				} else {
					reinsertDecl(ctx, decl);
				}
			} else if(auto* varDecl = llvm::dyn_cast<VarDecl>(decl)) {
				rewriter.RemoveText(varDecl->getSourceRange(), onlyRemoveOld);
				rewriter.InsertTextAfter(varDecl->getBeginLoc(), fmt::format("{};", *varDecl));
			} else {
				reinsertDecl(ctx, decl);
			}
		}

		return true;
	}
};

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
			logln("Transformation applied at: ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));             \
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
			logln("Transformation applied at: ", DisplaySourceLoc(ctxs.back().astContext, node->getBeginLoc()));             \
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

template <typename Visitor> class NormanASTConsumer final : public ASTConsumer {
	Visitor visitor;

public:
	explicit NormanASTConsumer(CompilerInstance* CI, Config const& config, Rewriter& rewriter)
	  : visitor{CI, config, rewriter} { }

	void HandleTranslationUnit(ASTContext& Context) { visitor.TraverseDecl(Context.getTranslationUnitDecl()); }
};

template <typename Consumer> class NormanFrontendAction final : public ASTFrontendAction {
	Config const& config;
	Rewriter rewriter;
	bool& rewritten;

public:
	NormanFrontendAction(bool* rewritten, Config const* config)
	  : config{*config}
	  , rewritten{*rewritten} { }

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) override {
		return std::make_unique<Consumer>(&CI, config, rewriter);
	}

	bool PrepareToExecuteAction(CompilerInstance& CI) override {
		rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return ASTFrontendAction::PrepareToExecuteAction(CI);
	}

	void EndSourceFileAction() override {
		if(RewriteBuffer const* rb = rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID())) {
#if LLVM_VERSION_MAJOR > 17
			auto fileEntryRef = rewriter.getSourceMgr().getFileEntryRefForID(rewriter.getSourceMgr().getMainFileID());
			assert(fileEntryRef.has_value());
			auto name = fileEntryRef->getName();
#else
			auto name = rewriter.getSourceMgr().getFileEntryForID(rewriter.getSourceMgr().getMainFileID())->getName();
#endif

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

	static llvm::cl::opt<std::uint64_t> maxIterations("n", llvm::cl::desc("Maximum number of iterations to run"),
	                                                  llvm::cl::init((std::numeric_limits<std::uint64_t>::max)()),
	                                                  llvm::cl::cat(NormanOptionCategory));

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

	{
		clang::tooling::ClangTool Tool{op->getCompilations(), op->getSourcePathList()[0]};
		// disable printing warnings
		Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-w"));

		bool rewritten;
		using TUFixupAction = NormanFrontendAction<NormanASTConsumer<TUFixupVisitor>>;
		int result = Tool.run(&(*newFrontendActionDataFactory<TUFixupAction>(&rewritten, &config)));
		if(result != EXIT_SUCCESS) {
			return result;
		}
	}

	bool rewritten = true;
	std::uint64_t count = 0;
	while(rewritten && count < maxIterations.getValue()) {
		clang::tooling::ClangTool Tool{op->getCompilations(), op->getSourcePathList()[0]};
		// disable printing warnings
		Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster("-w"));

		using NormaliseAction = NormanFrontendAction<NormanASTConsumer<NormaliseVisitor>>;
		int result = Tool.run(&(*newFrontendActionDataFactory<NormaliseAction>(&rewritten, &config)));
		if(result != EXIT_SUCCESS) {
			return result;
		}
		++count;
		logln(rewritten ? "File was rewritten" : "No change");
	}
	llvm::outs() << "Norman ran " << count << " times to normalize your source code.\n";
}
