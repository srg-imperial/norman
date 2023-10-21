#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T, typename U, typename... V>
	LogicalOr<std::decay_t<T>, std::decay_t<U>, std::decay_t<V>...> make_lor(T&& operand1, U&& operand2,
	                                                                         V&&... operands) {
		return {std::tuple<std::decay_t<T>, std::decay_t<U>, std::decay_t<V>...>{
		  std::forward<T>(operand1), std::forward<U>(operand2), std::forward<V>(operands)...}};
	}
} // namespace rewriting

template <typename... V> struct fmt::formatter<rewriting::LogicalOr<V...>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::LogicalOr<V...> const& node, FormatContext& ctx) {
		return do_format(ctx.out(), node.operands, std::integral_constant<std::size_t, 0>{});
	}

private:
	template <typename Out>
	Out do_format(Out out, std::tuple<V...> const& exprs, std::integral_constant<std::size_t, sizeof...(V) - 1>) {
		return format_one(out, std::get<sizeof...(V) - 1>(exprs));
	}

	template <std::size_t N, typename Out>
	Out do_format(Out out, std::tuple<V...> const& exprs, std::integral_constant<std::size_t, N>) {
		out = format_one(out, std::get<N>(exprs));
		out = fmt::format_to(out, FMT_COMPILE(" || "));
		return do_format(out, exprs, std::integral_constant<std::size_t, N + 1>{});
	}

	template <typename Out, typename T> Out format_one(Out out, T const& expr) {
		return fmt::format_to(out, FMT_COMPILE("({})"), expr);
	}

	template <typename Out, typename... W> Out format_one(Out out, rewriting::Identifier<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("{}"), expr);
	}

	template <typename Out, typename... W> Out format_one(Out out, rewriting::LogicalOr<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("{}"), expr);
	}
};
