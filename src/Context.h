#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <string>
#include <string_view>

struct Context {
	clang::ASTContext* astContext;
	std::string uidMarker;

	Context(clang::ASTContext* astContext, std::string uidMarker)
	  : astContext(astContext)
	  , uidMarker(std::move(uidMarker)) { }

	Context(Context const&) = delete;
	Context& operator=(Context const&) = delete;
	Context(Context&&) = default;
	Context& operator=(Context&&) = default;

	std::string uid(std::string_view prefix) const {
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

	auto source_text(clang::SourceRange source_range) const {
		return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(source_range),
		                                   astContext->getSourceManager(), astContext->getLangOpts());
	}
};