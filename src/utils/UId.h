#pragma once

#include <clang/AST/ASTContext.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <iterator>
#include <string>
#include <string_view>

namespace utils {
	inline std::string uid(clang::ASTContext* astContext, std::string prefix) {
		using namespace std::literals;

		std::string var_name = std::move(prefix);
		var_name += "_"sv;
		auto prefix_len = var_name.size();

		for(std::size_t id = astContext->Idents.size();; ++id) {
			fmt::format_to(std::back_inserter(var_name), FMT_COMPILE("{}"), id);
			if(astContext->Idents.find(var_name) == astContext->Idents.end()) {
				astContext->Idents.get(var_name);
				return var_name;
			}
			var_name.resize(prefix_len);
		}
	}
} // namespace utils