#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <optional>
#include <type_traits>

namespace rewriting {
	template <typename C, typename B> If<std::decay_t<C>, std::decay_t<B>, void> make_while(C&& condition, B&& body) {
		return {std::forward<C>(condition), std::forward<B>(body)};
	}
} // namespace rewriting

template <typename C, typename B> struct fmt::formatter<rewriting::While<C, B>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::While<C, B> const& node, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), FMT_COMPILE("while ({}) {{\n{}}}\n"), node.condition, node.body);
	}
};
