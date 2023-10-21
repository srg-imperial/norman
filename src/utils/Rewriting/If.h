#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <optional>
#include <type_traits>

namespace rewriting {
	template <typename C, typename T> If<std::decay_t<C>, std::decay_t<T>, void> make_if(C&& condition, T&& then_branch) {
		return {std::forward<C>(condition), std::forward<T>(then_branch)};
	}

	template <typename C, typename T, typename E>
	If<std::decay_t<C>, std::decay_t<T>, std::decay_t<E>> make_if(C&& condition, T&& then_branch, E&& else_branch) {
		return {
		  std::forward<C>(condition),
		  std::forward<T>(then_branch),
		  std::forward<E>(else_branch),
		};
	}
} // namespace rewriting

template <typename C, typename T, typename E> struct fmt::formatter<rewriting::If<C, T, E>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::If<C, T, E> const& node, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), FMT_COMPILE("if ({}) {{\n{}}} else {{\n{}}}\n"), node.condition, node.then_branch,
		                      node.else_branch);
	}
};

template <typename C, typename T, typename E> struct fmt::formatter<rewriting::If<C, T, std::optional<E>>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::If<C, T, E> const& node, FormatContext& ctx) {
		if(node.else_branch) {
			return fmt::format_to(
			  ctx.out(), FMT_COMPILE("{}"),
			  rewriting::If<C const&, T const&, E const&>{node.condition, node.then_branch, *node.else_branch});
		} else {
			return fmt::format_to(ctx.out(), FMT_COMPILE("{}"),
			                      rewriting::If<C const&, T const&, void>{node.condition, node.then_branch});
		}
	}
};

template <typename C, typename T> struct fmt::formatter<rewriting::If<C, T, void>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::If<C, T, void> const& node, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), FMT_COMPILE("if ({}) {{\n{}}}\n"), node.condition, node.then_branch);
	}
};
