#pragma once

#include <algorithm>
#include <fmt/core.h>
#include <llvm/ADT/StringRef.h>

template <> struct fmt::formatter<llvm::StringRef> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(llvm::StringRef const& str, FormatContext& ctx) const {
		return std::copy(str.begin(), str.end(), ctx.out());
	}
};
