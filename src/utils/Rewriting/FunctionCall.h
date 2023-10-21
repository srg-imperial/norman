#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T, typename... V>
	FunctionCall<std::decay_t<T>, std::decay_t<V>...> make_call(T&& function, V&&... arguments) {
		return {std::forward<T>(function), std::tuple<std::decay_t<V>...>{std::forward<V>(arguments)...}};
	}
} // namespace rewriting

template <typename T, typename... V> struct fmt::formatter<rewriting::FunctionCall<T, V...>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::FunctionCall<T, V...> const& node, FormatContext& ctx) {
		auto out = format_fn(ctx.out(), node.function);
		out = fmt::format_to(out, FMT_COMPILE("("));
		if constexpr(sizeof...(V) > 0) {
			out = format_args(ctx.out(), node.arguments, std::integral_constant<std::size_t, 0>{});
		}
		return fmt::format_to(out, FMT_COMPILE(")"));
	}

private:
	template <typename Out, typename U> Out format_fn(Out out, U const& fn) {
		return fmt::format_to(out, FMT_COMPILE("({})"), fn);
	}

	template <typename Out, typename... W> Out format_fn(Out out, rewriting::Identifier<W...> const& fn) {
		return fmt::format_to(out, FMT_COMPILE("{}"), fn);
	}

	template <typename Out, typename U, typename... W>
	Out format_fn(Out out, rewriting::FunctionCall<U, W...> const& fn) {
		return fmt::format_to(out, FMT_COMPILE("{}"), fn);
	}

	template <typename Out>
	Out format_args(Out out, std::tuple<V...> const& args, std::integral_constant<std::size_t, sizeof...(V) - 1>) {
		return format_arg(out, std::get<sizeof...(V) - 1>(args));
	}

	template <std::size_t N, typename Out>
	Out format_args(Out out, std::tuple<V...> const& args, std::integral_constant<std::size_t, N>) {
		out = format_arg(out, std::get<N>(args));
		return format_args(out, args, std::integral_constant<std::size_t, N + 1>{});
	}

	template <typename Out, typename U> Out format_arg(Out out, U const& arg) {
		// `arg` might be a comma expression in the general case
		return fmt::format_to(out, FMT_COMPILE("({})"), arg);
	}
};
