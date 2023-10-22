#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T, typename U>
	Assignment<std::decay_t<T>, std::decay_t<U>> make_assign(T&& operand1, U&& operand2) {
		return {std::forward<T>(operand1), std::forward<U>(operand2)};
	}
} // namespace rewriting

template <typename T, typename U> struct fmt::formatter<rewriting::Assignment<T, U>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::Assignment<T, U> const& node, FormatContext& ctx) {
		auto out = format_lhs(ctx.out(), node.operand1);
		out = fmt::format_to(out, FMT_COMPILE(" = "));
		return format_rhs(out, node.operand2);
	}

private:
	template <typename Out, typename E> Out format_lhs(Out out, E const& expr) {
		return fmt::format_to(out, FMT_COMPILE("({})"), expr);
	}

	template <typename Out, typename... W> Out format_lhs(Out out, rewriting::Identifier<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("{}"), expr);
	}
	template <typename Out, typename E> Out format_rhs(Out out, E const& expr) {
		return fmt::format_to(out, FMT_COMPILE("({})"), expr);
	}

	template <typename Out, typename... W> Out format_rhs(Out out, rewriting::Identifier<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("{}"), expr);
	}
};
