#include "Context.h"

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Lex/Lexer.h>

#include <fmt/compile.h>
#include <fmt/format.h>

namespace {
	struct UsedLabels : clang::RecursiveASTVisitor<UsedLabels> {
		std::set<clang::LabelDecl*> usedLabelDecls{};

    bool TraverseGotoStmt(clang::GotoStmt* gotoStmt) {
      usedLabelDecls.insert(gotoStmt->getLabel());

      return true;
    }
	};
} // namespace

Context Context::FileLevel(clang::ASTContext& astContext) noexcept { return {&astContext, {}, {}}; }
Context Context::FunctionLevel(clang::ASTContext& astContext, clang::FunctionDecl& fdecl) {
  UsedLabels usedLabels;
  usedLabels.TraverseFunctionDecl(&fdecl);
	return {&astContext, fdecl.getName().str(), std::move(usedLabels.usedLabelDecls)};
}

std::string Context::uid(std::string_view prefix) const {
	std::string var_name = fmt::format(FMT_COMPILE("{}_{}_"), prefix, uidMarker);
	auto prefix_len = var_name.size();

	for(std::size_t id = 1;; ++id) {
		fmt::format_to(std::back_inserter(var_name), FMT_COMPILE("{:x}"), id);
		if(astContext->Idents.find(var_name) == astContext->Idents.end()) {
			astContext->Idents.get(var_name);
			return var_name;
		}
		var_name.resize(prefix_len);
	}
}

llvm::StringRef Context::source_text(clang::SourceRange source_range) const {
	return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(source_range),
	                                   astContext->getSourceManager(), astContext->getLangOpts());
}