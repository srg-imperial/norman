#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <optional>
#include <type_traits>

namespace rewriting {
	template <typename T, typename N>
	VariableDeclaration<std::decay_t<T>, std::decay_t<N>, void> make_vardecl(T&& type, N&& name) {
		return {std::forward<T>(type), std::forward<N>(name)};
	}

	template <typename T, typename N, typename V>
	VariableDeclaration<std::decay_t<T>, std::decay_t<N>, std::decay_t<V>> make_vardecl(T&& type, N&& name, V&& value) {
		return {
		  std::forward<T>(type),
		  std::forward<N>(name),
		  std::forward<V>(value),
		};
	}
} // namespace rewriting

template <typename T, typename N, typename V> struct fmt::formatter<rewriting::VariableDeclaration<T, N, V>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(rewriting::VariableDeclaration<T, N, V> const& node, FormatContext& ctx) {
		auto out = fmt::format_to(ctx.out(), FMT_COMPILE("{} {} = "), node.type, node.name);
		out = format_val(out, node.value);
		return fmt::format_to(ctx.out(), FMT_COMPILE(";\n"));
	}

private:
	template <typename Out, typename U> Out format_val(Out out, U const& value) {
		return fmt::format_to(out, FMT_COMPILE("({})"), value);
	}

	template <typename Out, typename... W> Out format_val(Out out, rewriting::Identifier<W...> const& value) {
		return fmt::format_to(out, FMT_COMPILE("{}"), value);
	}

	template <typename Out> Out format_val(Out out, int const& value) {
		return fmt::format_to(out, FMT_COMPILE("{}"), value);
	}
};

template <typename T, typename N, typename V>
struct fmt::formatter<rewriting::VariableDeclaration<T, N, std::optional<V>>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(rewriting::VariableDeclaration<T, N, V> const& node, FormatContext& ctx) {
		if(node.value) {
			return fmt::format_to(
			  ctx.out(), FMT_COMPILE("{}"),
			  rewriting::VariableDeclaration<T const&, N const&, V const&>{node.type, node.name, *node.value});
		} else {
			return fmt::format_to(ctx.out(), FMT_COMPILE("{}"),
			                      rewriting::VariableDeclaration<T const&, N const&, void>{node.type, node.name});
		}
	}
};

template <typename T, typename N> struct fmt::formatter<rewriting::VariableDeclaration<T, N, void>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(rewriting::VariableDeclaration<T, N, void> const& node, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), FMT_COMPILE("{} {};\n"), node.type, node.name);
	}
};
