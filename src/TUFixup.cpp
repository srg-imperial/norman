#include "TUFixup.h"

#include "Context.h"
#include "util/frontend.h"

#include "util/fmtlib_clang.h"
#include "util/fmtlib_llvm.h"
#include <format>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/Casting.h>

#include <memory>
#include <string>

#if LLVM_VERSION_MAJOR < 18
	#include <regex>
#endif

namespace {
	class TUFixupVisitor : public clang::RecursiveASTVisitor<TUFixupVisitor> {
		std::string& result;

	public:
		explicit TUFixupVisitor(clang::CompilerInstance*, Config const&, std::string& result)
		  : result(result) { }

		void writeDecl(Context const& ctx, clang::Decl* decl) {
			std::format_to(std::back_inserter(result), "{};\n", *decl);
		}

		bool TraverseTranslationUnitDecl(clang::TranslationUnitDecl* tuDecl) {
			auto ctx = Context::FileLevel(tuDecl->getASTContext());

			for(auto* decl : tuDecl->decls()) {
				if(decl->isImplicit()) {
					// do nothing, it will be explicit for the resulting code as well
				} else if(auto* recordDecl = llvm::dyn_cast<clang::RecordDecl>(decl)) {
					if(recordDecl->getName().empty()) {
						auto id = &ctx.astContext->Idents.get(ctx.uid("Decl"));
						recordDecl->setDeclName(clang::DeclarationName{id});
					}
					writeDecl(ctx, recordDecl);
				} else if(auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(decl)) {
					if(enumDecl->getName().empty()) {
						auto id = &ctx.astContext->Idents.get(ctx.uid("Decl"));
						enumDecl->setDeclName(clang::DeclarationName{id});
					}
					writeDecl(ctx, enumDecl);
				} else {
					writeDecl(ctx, decl);
				}
			}

			return true;
		}

#if LLVM_VERSION_MAJOR < 18
		static void finalize(Config const&, std::string& result) {
			// Clang < 18 does not correctly print `__attribute__((unused))`.
			std::regex attribute_unused{R"(__attribute__\s*\(\s*\(\s*unused\s*\)\s*\))"};
			std::string new_result;
			new_result.reserve(result.size());
			std::regex_replace(std::back_inserter(new_result), result.begin(), result.end(), attribute_unused, "");

			result = std::move(new_result);
		}
#endif
	};
} // namespace

std::unique_ptr<clang::tooling::FrontendActionFactory> newTUFixupFrontendFactory(Config& config) {
	using Action = StringRewriterFrontendAction<SimpleASTConsumer<TUFixupVisitor>>;
	return std::make_unique<FrontendActionDataFactory<Action, Config*>>(&config);
}