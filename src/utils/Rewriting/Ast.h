#pragma once

#include <tuple>

namespace rewriting {
	template <typename C, typename B> struct While {
		C condition;
		B body;
	};

	template <typename C, typename T, typename E> struct If {
		C condition;
		T then_branch;
		E else_branch;
	};

	template <typename C, typename T> struct If<C, T, void> {
		C condition;
		T then_branch;
	};

	template <typename T> struct Not { T operand; };

	template <typename T, typename U> struct Assignment {
		T operand1;
		U operand2;
	};

	template <typename T, typename U> struct Equal {
		T operand1;
		U operand2;
	};

	template <typename T, typename U> struct NotEqual {
		T operand1;
		U operand2;
	};

	template <typename... V> struct LogicalAnd {
		static_assert(sizeof...(V) > 1, "A logical and requires at least two operands");
		std::tuple<V...> operands;
	};

	template <typename... V> struct LogicalOr {
		static_assert(sizeof...(V) > 1, "A logical or requires at least two operands");
		std::tuple<V...> operands;
	};

	template <typename T, typename... V> struct FunctionCall {
		T function;
		std::tuple<V...> arguments;
	};

	template <typename... V> struct Statements { std::tuple<V...> statements; };

	template <typename... V> struct Identifier {
		static_assert(sizeof...(V) > 1, "An identifier requires at least one part");
		std::tuple<V...> parts;
	};

	template <typename T, typename N, typename V> struct VariableDeclaration {
		T type;
		N name;
		V value;
	};

	template <typename T, typename N> struct VariableDeclaration<T, N, void> {
		T type;
		N name;
	};
} // namespace rewriting
