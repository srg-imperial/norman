#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T, typename... V>
	Identifier<std::decay_t<T>, std::decay_t<V>...> make_id(T&& part1, V&&... parts) {
		return {std::tuple<std::decay_t<T>, std::decay_t<V>...>{std::forward<T>(part1), std::forward<V>(parts)...}};
	}
} // namespace rewriting

template <typename... V> struct fmt::formatter<rewriting::Identifier<V...>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::Identifier<V...> const& node, FormatContext& ctx) {
		return do_format(ctx.out(), node.parts, std::integral_constant<std::size_t, 0>{});
	}

private:
	template <typename Out>
	Out do_format(Out out, std::tuple<V...> const& exprs, std::integral_constant<std::size_t, sizeof...(V) - 1>) {
		return fmt::format_to(out, FMT_COMPILE("{}"), std::get<sizeof...(V) - 1>(exprs));
	}

	template <std::size_t N, typename Out>
	Out do_format(Out out, std::tuple<V...> const& exprs, std::integral_constant<std::size_t, N>) {
		out = fmt::format_to(out, FMT_COMPILE("{}"), std::get<N>(exprs));
		return do_format(out, exprs, std::integral_constant<std::size_t, N + 1>{});
	}
};
