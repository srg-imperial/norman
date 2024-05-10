#include "Config.h"
#include "Normalise.h"
#include "TUFixup.h"
#include "util/Log.h"

#include <clang/Driver/Options.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/InitLLVM.h>

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <utility>

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

		auto actionFactory = newTUFixupFrontendFactory(config);
		int result = Tool.run(actionFactory.get());
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

		auto actionFactory = newNormaliseFrontendFactory(rewritten, config);
		int result = Tool.run(actionFactory.get());
		if(result != EXIT_SUCCESS) {
			return result;
		}
		++count;
		logln(rewritten ? "File was rewritten" : "No change");
	}
	llvm::outs() << "Norman ran " << count << " times to normalize your source code.\n";
}
