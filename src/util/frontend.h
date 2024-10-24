#pragma once

#include "../Config.h"

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/FileSystem.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

template <typename Visitor> class SimpleASTConsumer final : public clang::ASTConsumer {
	Visitor visitor;

	template <typename... Args> struct has_finalize_helper {
		template <typename T, typename = void> static std::false_type has_finalize(long);
		template <typename T, typename = std::void_t<decltype(T::finalize(std::declval<Args>()...))>>
		static std::true_type has_finalize(int);
	};

	template <typename... Args>
	struct has_finalize : decltype(has_finalize_helper<Args...>::template has_finalize<Visitor>(0)) { };

public:
	template <typename... V>
	explicit SimpleASTConsumer(V&&... args)
	  : visitor{std::forward<V>(args)...} { }

	void HandleTranslationUnit(clang::ASTContext& Context) { visitor.TraverseDecl(Context.getTranslationUnitDecl()); }

	template <typename... Args> static void finalize(Args&&... args) {
		if constexpr(has_finalize<Args...>::value) {
			return Visitor::finalize(std::forward<Args>(args)...);
		}
	}
};

template <typename Consumer> class StringRewriterFrontendAction final : public clang::ASTFrontendAction {
	Config const& config;
	std::string result;

	clang::SourceManager* sourceManager;

public:
	StringRewriterFrontendAction(Config const* config)
	  : config{*config} { }

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
	                                                      llvm::StringRef /*file*/) override {
		return std::make_unique<Consumer>(&CI, config, result);
	}

	bool PrepareToExecuteAction(clang::CompilerInstance& CI) override {
		result.clear();
		sourceManager = &CI.getSourceManager();
		return clang::ASTFrontendAction::PrepareToExecuteAction(CI);
	}

	void EndSourceFileAction() override {
		Consumer::finalize(config, result);

#if LLVM_VERSION_MAJOR > 17
		auto fileEntryRef = sourceManager->getFileEntryRefForID(sourceManager->getMainFileID());
		assert(fileEntryRef.has_value());
		auto name = fileEntryRef->getName();
#else
		auto name = sourceManager->getFileEntryForID(sourceManager->getMainFileID())->getName();
#endif

		std::error_code error_code;
		llvm::raw_fd_ostream outFile{name, error_code};
		if(error_code) {
			llvm::errs() << error_code.message() << "\n";
			std::exit(1);
		}
		outFile.write(result.data(), result.size());
		outFile.close();
	}
};

template <typename Consumer> class ClangRewriterFrontendAction final : public clang::ASTFrontendAction {
	bool& rewritten;
	Config const& config;
	clang::Rewriter rewriter;

public:
	ClangRewriterFrontendAction(bool* rewritten, Config const* config)
	  : config{*config}
	  , rewritten{*rewritten} { }

	std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI,
	                                                      llvm::StringRef /*file*/) override {
		return std::make_unique<Consumer>(&CI, config, rewriter);
	}

	bool PrepareToExecuteAction(clang::CompilerInstance& CI) override {
		rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return clang::ASTFrontendAction::PrepareToExecuteAction(CI);
	}

	void EndSourceFileAction() override {
		Consumer::finalize(config, rewriter);

		if(clang::RewriteBuffer const* rb = rewriter.getRewriteBufferFor(rewriter.getSourceMgr().getMainFileID())) {
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

	template <std::size_t... I> std::unique_ptr<clang::FrontendAction> create_impl(std::index_sequence<I...>) {
		return std::make_unique<T>(std::get<I>(args)...);
	}

public:
	FrontendActionDataFactory(V&&... args)
	  : args(std::forward<V>(args)...) { }

	std::unique_ptr<clang::FrontendAction> create() override { return create_impl(std::index_sequence_for<V...>{}); }
};

template <typename T, typename... V>
std::unique_ptr<clang::tooling::FrontendActionFactory> newFrontendActionDataFactory(V&&... args) {
	return std::make_unique<FrontendActionDataFactory<T, V...>>(std::forward<V>(args)...);
}