#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>

namespace rewriting {
	template <typename... V> Statements<std::decay_t<V>...> make_stmts(V&&... statement) {
		return {std::tuple<std::decay_t<V>...>{std::forward<V>(statement)...}};
	}
} // namespace rewriting

template <typename... V> struct fmt::formatter<rewriting::Statements<V...>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::Statements<V...> const& node, FormatContext& ctx) {
		if constexpr(sizeof...(V) > 0) {
			return do_format(ctx.out(), node.statements, std::integral_constant<std::size_t, 0>{});
		} else {
			return ctx.out();
		}
	}

private:
	template <typename Out>
	Out do_format(Out out, std::tuple<V...> const& stmts, std::integral_constant<std::size_t, sizeof...(V) - 1>) {
		return format_one(out, std::get<sizeof...(V) - 1>(stmts));
	}

	template <std::size_t N, typename Out>
	Out do_format(Out out, std::tuple<V...> const& stmts, std::integral_constant<std::size_t, N>) {
		out = format_one(out, std::get<N>(stmts));
		return do_format(out, stmts, std::integral_constant<std::size_t, N + 1>{});
	}

	template <typename U, typename Out> Out format_one(Out out, U const& stmt) {
		auto format_to_n_result = fmt::format_to_n(out, (std::numeric_limits<std::size_t>::max)(), FMT_COMPILE("{}"), stmt);
		out = format_to_n_result.out;
		if(format_to_n_result.size) {
			out = fmt::format_to(out, FMT_COMPILE(";\n"));
		}
		return out;
	}

	template <typename Out, typename... W> Out format_one(Out out, rewriting::If<W...> const& stmt) {
		return fmt::format_to(out, FMT_COMPILE("{}"), stmt);
	}

	template <typename Out, typename A, typename B, typename C>
	Out format_one(Out out, rewriting::VariableDeclaration<A, B, C> const& stmt) {
		return fmt::format_to(out, FMT_COMPILE("{}"), stmt);
	}
};
