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
#include "transform/StringLiteral.h"
#include "transform/SwitchStmt.h"
#include "transform/WhileStmt.h"
#include "util/FunctionFilter.h"
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
	ASTContext* astContext;
	FunctionFilter const& functionFilter;
	Rewriter& rewriter;
	Rewriter::RewriteOptions onlyRemoveOld;
	std::vector<std::string> to_hoist;

public:
	explicit NormaliseExprVisitor(CompilerInstance* CI, FunctionFilter const& functionFilter, Rewriter& rewriter)
	  : astContext(&(CI->getASTContext()))
	  , functionFilter(functionFilter)
	  , rewriter(rewriter) {
		onlyRemoveOld.IncludeInsertsAtBeginOfRange = false;
		onlyRemoveOld.IncludeInsertsAtEndOfRange = false;
	}

	bool TraverseFunctionDecl(FunctionDecl* fdecl) {
		if(functionFilter.skip(fdecl)) {
			logln("Skipping function ", fdecl->getName(), DisplaySourceLoc(astContext, fdecl->getBeginLoc()));
			return true;
		}

		return RecursiveASTVisitor::TraverseFunctionDecl(fdecl);
	}

	bool TraverseEnumDecl(EnumDecl*) { return true; }

	bool TraverseCallExpr(CallExpr* cexpr) {
		if(!TraverseStmt(cexpr->getCallee())) {
			return false;
		}

		for(unsigned i = 0, num_args = cexpr->getNumArgs(); i < num_args; i++) {
			Expr* arg = cexpr->getArg(i);
			if(auto output_pair = transform::transformCallArg(astContext, arg)) {
				if(!output_pair->to_hoist.empty()) {
					to_hoist.emplace_back(std::move(output_pair->to_hoist));
				}
				rewriter.RemoveText(arg->getSourceRange(), onlyRemoveOld);
				rewriter.InsertTextAfter(arg->getSourceRange().getBegin(), output_pair->expression);

				logln("Transformation applied at: ", DisplaySourceLoc(astContext, arg->getBeginLoc()));
			} else {
				if(!RecursiveASTVisitor<NormaliseExprVisitor>::TraverseStmt(arg)) {
					return false;
				}
			}
		}

		return true;
	}

	bool TraverseNullStmt(NullStmt* nullstmt) {
		rewriter.RemoveText(nullstmt->getSourceRange(), onlyRemoveOld);
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
		logFnScope("for source line ", DisplaySourceLoc(astContext, node->getBeginLoc()));                                 \
                                                                                                                       \
		if(auto output_pair = transform::transform##kind(astContext, node)) {                                              \
			if(!output_pair->to_hoist.empty()) {                                                                             \
				to_hoist.emplace_back(std::move(output_pair->to_hoist));                                                       \
			}                                                                                                                \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), output_pair->expression);                            \
                                                                                                                       \
			logln("Transformation applied at: ", DisplaySourceLoc(astContext, node->getBeginLoc()));                         \
			return true;                                                                                                     \
		} else {                                                                                                           \
			return RecursiveASTVisitor<NormaliseExprVisitor>::Traverse##type(node);                                          \
		}                                                                                                                  \
	}

#define TraverseStmtFn(type)                                                                                           \
	bool Traverse##type(type* node) {                                                                                    \
		logFnScope("for source line ", DisplaySourceLoc(astContext, node->getBeginLoc()));                                 \
                                                                                                                       \
		if(auto output = transform::transform##type(astContext, node)) {                                                   \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), *output);                                            \
                                                                                                                       \
			logln("Transformation applied at: ", DisplaySourceLoc(astContext, node->getBeginLoc()));                         \
			return true;                                                                                                     \
		} else {                                                                                                           \
			return RecursiveASTVisitor<NormaliseExprVisitor>::Traverse##type(node);                                          \
		}                                                                                                                  \
	}

	TraverseExprFn(CommaOperator, BinaryOperator);
	TraverseExprFn(ConditionalOperator, ConditionalOperator);
	TraverseExprFn(LAndOperator, BinaryOperator);
	TraverseExprFn(LOrOperator, BinaryOperator);
	TraverseExprFn(ParenExpr, ParenExpr);
	TraverseExprFn(StringLiteral, StringLiteral);

	TraverseStmtFn(DoStmt);
	TraverseStmtFn(ForStmt);
	TraverseStmtFn(IfStmt);
	TraverseStmtFn(ReturnStmt);
	TraverseStmtFn(SwitchStmt);
	TraverseStmtFn(WhileStmt);

	bool TraverseCompoundStmt(CompoundStmt* cstmt) {
		for(auto* stmt : cstmt->body()) {
			assert(to_hoist.empty());

			while(auto labelStmt = dyn_cast<LabelStmt>(stmt)) {
				stmt = labelStmt->getSubStmt();
			}
			while(auto parenExpr = dyn_cast<ParenExpr>(stmt)) {
				rewriter.RemoveText(parenExpr->getLParen(), onlyRemoveOld);
				rewriter.RemoveText(parenExpr->getRParen(), onlyRemoveOld);
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
};

class NormaliseExprASTConsumer final : public ASTConsumer {
	NormaliseExprVisitor visitor;

public:
	explicit NormaliseExprASTConsumer(CompilerInstance* CI, FunctionFilter const& functionFilter, Rewriter& rewriter)
	  : visitor{CI, functionFilter, rewriter} { }

	void HandleTranslationUnit(ASTContext& Context) { visitor.TraverseDecl(Context.getTranslationUnitDecl()); }
};

class NormaliseExprFrontendAction final : public ASTFrontendAction {
	std::string_view output;
	FunctionFilter const& functionFilter;
	Rewriter rewriter;

public:
	NormaliseExprFrontendAction(std::string_view output, FunctionFilter const& functionFilter)
	  : output(output)
	  , functionFilter(functionFilter) { }

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) override {
		return std::make_unique<NormaliseExprASTConsumer>(&CI, functionFilter, rewriter);
	}

	bool PrepareToExecuteAction(CompilerInstance& CI) override {
		rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return ASTFrontendAction::PrepareToExecuteAction(CI);
	}

	void EndSourceFileAction() override {
		if(RewriteBuffer const* rb = rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID())) {
			std::error_code error_code;
			llvm::raw_fd_ostream outFile{output, error_code};
			if(error_code) {
				logln(error_code.message());
				std::exit(1);
			}
			rb->write(outFile);
			outFile.close();
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
	static llvm::cl::OptionCategory NormaliseExprCategory("Norman's Options");

	static llvm::cl::opt<std::string> filter("filter", llvm::cl::desc("Path to the file containing the function filter"),
	                                         llvm::cl::cat(NormaliseExprCategory), llvm::cl::Required);
	static llvm::cl::opt<std::string> Output("o", llvm::cl::desc("Where to place the output"),
	                                         llvm::cl::cat(NormaliseExprCategory), llvm::cl::Required);

	llvm::cl::HideUnrelatedOptions(NormaliseExprCategory);

#if LLVM_VERSION_MAJOR > 12
	auto op = clang::tooling::CommonOptionsParser::create(argc, argv, NormaliseExprCategory);
	if(!op) {
		logln("Could not create options parser!\n");
		return EXIT_FAILURE;
	}
	clang::tooling::ClangTool Tool(op->getCompilations(), op->getSourcePathList()[0]);
#else
	clang::tooling::CommonOptionsParser op(argc, argv, NormaliseExprCategory);
	clang::tooling::ClangTool Tool(op.getCompilations(), op.getSourcePathList()[0]);
#endif

	if(auto bl = FunctionFilter::from_file(filter.getValue())) {
		int result =
		  Tool.run(&(*newFrontendActionDataFactory<NormaliseExprFrontendAction>(std::string_view{Output}, std::move(*bl))));

		return result;
	} else {
		logln("Could not open filter file!\n");
		return EXIT_FAILURE;
	}
}
