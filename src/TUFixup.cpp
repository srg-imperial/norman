#include "TUFixup.h"

#include "Context.h"
#include "util/frontend.h"

#include "util/fmtlib_clang.h"
#include "util/fmtlib_llvm.h"
#include <fmt/format.h>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/Casting.h>

#include <memory>
#include <string>

namespace {
	class TUFixupVisitor : public clang::RecursiveASTVisitor<TUFixupVisitor> {
		std::string& result;

	public:
		explicit TUFixupVisitor(clang::CompilerInstance*, Config const&, std::string& result)
		  : result(result) { }

		void writeDecl(Context const& ctx, clang::Decl* decl) {
			fmt::format_to(std::back_inserter(result), "{};\n", *decl);
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
	};
} // namespace

std::unique_ptr<clang::tooling::FrontendActionFactory> newTUFixupFrontendFactory(Config& config) {
	using Action = StringRewriterFrontendAction<SimpleASTConsumer<TUFixupVisitor>>;
	return std::make_unique<FrontendActionDataFactory<Action, Config*>>(&config);
}