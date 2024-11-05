#pragma once

#include <algorithm>
#include <clang/AST/AST.h>
#include <fmt/core.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <type_traits>

template <typename T, typename Char>
struct fmt::formatter<T, Char, std::enable_if_t<std::is_base_of_v<clang::Decl, T>>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(T const& decl, FormatContext& ctx) const {
		std::string buffer;
		{
			llvm::raw_string_ostream out{buffer};
			decl.print(out);
		}
		return std::copy(buffer.begin(), buffer.end(), ctx.out());
	}
};
