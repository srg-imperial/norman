#pragma once

#include <clang/AST/ASTContext.h>
#include <llvm/ADT/StringRef.h>

#include <set>
#include <string>
#include <string_view>

struct Context {
	clang::ASTContext* astContext;
	std::string uidMarker;
	std::set<clang::LabelDecl*> usedLabels;

	static Context FileLevel(clang::ASTContext& astContext) noexcept;
	static Context FunctionLevel(clang::ASTContext& astContext, clang::FunctionDecl& fdecl);

private:
	Context(clang::ASTContext* astContext, std::string uidMarker, std::set<clang::LabelDecl*> usedLabels)
	  : astContext(astContext)
	  , uidMarker(std::move(uidMarker))
	  , usedLabels(std::move(usedLabels)) { }

public:
	Context(Context const&) = delete;
	Context& operator=(Context const&) = delete;
	Context(Context&&) = default;
	Context& operator=(Context&&) = default;

	std::string uid(std::string_view prefix) const;
	llvm::StringRef source_text(clang::SourceRange source_range) const;
	llvm::StringRef source_text(clang::SourceLocation start, clang::SourceLocation end) const {
		return source_text(clang::SourceRange{start, end});
	}
};