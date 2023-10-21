#pragma once

#include "Ast.h"

#include <fmt/compile.h>
#include <fmt/format.h>
#include <type_traits>

namespace rewriting {
	template <typename T> Not<std::decay_t<T>> make_not(T&& operand) { return {std::forward<T>(operand)}; }
} // namespace rewriting

template <typename T> struct fmt::formatter<rewriting::Not<T>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(rewriting::Not<T> const& node, FormatContext& ctx) {
		return do_format(ctx.out(), node.operand);
	}

private:
	template <typename Out, typename E> Out do_format(Out out, E const& expr) {
		return fmt::format_to(out, FMT_COMPILE("!({})"), expr);
	}

	template <typename Out, typename... W> Out do_format(Out out, rewriting::Identifier<W...> const& expr) {
		return fmt::format_to(out, FMT_COMPILE("!{}"), expr);
	}
};
