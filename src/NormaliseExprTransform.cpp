#include "CallArg.h"
#include "CommaOperator.h"
#include "ConditionalOperator.h"
#include "DoStmt.h"
#include "ForStmt.h"
#include "IfStmt.h"
#include "LAndOperator.h"
#include "LOrOperator.h"
#include "ParenExpr.h"
#include "ReturnStmt.h"
#include "StringLiteral.h"
#include "SwitchStmt.h"
#include "WhileStmt.h"

#include "utils/FunctionFilter.h"
#include "utils/Log.h"
#include "utils/NumberRange.h"

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

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace clang;

Rewriter rewriter;

bool rewritten = false; // flag to be set if our normalisation pass was able to rewrite anything in the original file

class NormaliseExprVisitor : public RecursiveASTVisitor<NormaliseExprVisitor> {
	ASTContext* astContext;
	FunctionFilter const& functionFilter;
	Rewriter::RewriteOptions onlyRemoveOld;
	std::vector<std::string> to_hoist;

public:
	explicit NormaliseExprVisitor(CompilerInstance* CI, FunctionFilter const& functionFilter)
	  : astContext(&(CI->getASTContext()))
	  , functionFilter(functionFilter) {
		rewriter.setSourceMgr(astContext->getSourceManager(), astContext->getLangOpts());
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
			if(auto output_pair = transformCallArg(astContext, arg)) {
				if(!output_pair->to_hoist.empty()) {
					to_hoist.emplace_back(std::move(output_pair->to_hoist));
				}
				rewriter.RemoveText(arg->getSourceRange(), onlyRemoveOld);
				rewriter.InsertTextAfter(arg->getSourceRange().getBegin(), output_pair->expression);
				rewritten = true;

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
		if(auto output_pair = transform##kind(astContext, node)) {                                                         \
			if(!output_pair->to_hoist.empty()) {                                                                             \
				to_hoist.emplace_back(std::move(output_pair->to_hoist));                                                       \
			}                                                                                                                \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), output_pair->expression);                            \
			rewritten = true;                                                                                                \
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
		if(auto output = transform##type(astContext, node)) {                                                              \
			rewriter.RemoveText(node->getSourceRange(), onlyRemoveOld);                                                      \
			rewriter.InsertTextAfter(node->getSourceRange().getBegin(), *output);                                            \
			rewritten = true;                                                                                                \
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
	explicit NormaliseExprASTConsumer(CompilerInstance* CI, FunctionFilter const& functionFilter)
	  : visitor{CI, functionFilter} { }

	void HandleTranslationUnit(ASTContext& Context) { visitor.TraverseDecl(Context.getTranslationUnitDecl()); }
};

class NormaliseExprFrontendAction : public ASTFrontendAction {
	std::string_view output;
	FunctionFilter const& functionFilter;

public:
	NormaliseExprFrontendAction(std::string_view output, FunctionFilter const& functionFilter)
	  : output(output)
	  , functionFilter(functionFilter) { }

	virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance& CI, StringRef /*file*/) final {
		return std::unique_ptr<clang::ASTConsumer>(new NormaliseExprASTConsumer(&CI, functionFilter));
	}

	void EndSourceFileAction() override {
		if(!rewritten) { // early exit if nothing has been rewritten so as to not try to make a buffer etc.
			return;
		}
		SourceManager& SM = rewriter.getSourceMgr();
		const RewriteBuffer* rb = rewriter.getRewriteBufferFor(SM.getMainFileID());

		if(rb) {
			std::error_code error_code;
#if LLVM_VERSION_MAJOR > 12
			llvm::raw_fd_ostream outFile(output, error_code, llvm::sys::fs::OF_None);
#else
			llvm::raw_fd_ostream outFile(output, error_code, llvm::sys::fs::F_None);
#endif
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
	return std::unique_ptr<clang::tooling::FrontendActionFactory>(
	  new FrontendActionDataFactory<T, V...>{std::forward<V>(args)...});
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
