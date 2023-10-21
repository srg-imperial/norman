#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T, typename U> Equal<std::decay_t<T>, std::decay_t<U>> make_eq(T&& operand1, U&& operand2) {
		return {std::forward<T>(operand1), std::forward<U>(operand2)};
	}
} // namespace rewriting

template <typename T, typename U> struct fmt::formatter<rewriting::Equal<T, U>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::Equal<T, U> const& node, FormatContext& ctx) {
		auto out = format_one(ctx.out(), node.operand1);
		out = fmt::format_to(out, FMT_COMPILE(" == "));
		return format_one(out, node.operand2);
	}

private:
	template <typename Out, typename E> Out format_one(Out out, E const& expr) {
		return fmt::format_to(out, FMT_COMPILE("({})"), expr);
	}

	template <typename Out, typename... W> Out format_one(Out out, rewriting::Identifier<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("{}"), expr);
	}
};
