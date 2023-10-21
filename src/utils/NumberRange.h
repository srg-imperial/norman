#pragma once

#include <cassert>
#include <fmt/format.h>
#include <type_traits>

template <typename T> class InclusiveNumberRange {
	T first;
	T last;

public:
	constexpr InclusiveNumberRange(T first, T last) noexcept(noexcept(static_cast<T>(std::move(first))))
	  : first(std::move(first))
	  , last(std::move(last)) { }

	inline constexpr bool contains(T const& n) const { return first <= n && n <= last; }

	template <typename OS> friend OS& operator<<(OS& out, InclusiveNumberRange const& self) {
		out << "[" << self.first << ", " << self.last << "]";
		return out;
	}

	class Iterator {
		T n;

	public:
		constexpr Iterator(T n) noexcept(noexcept(static_cast<T>(std::move(n))))
		  : n(std::move(n)) { }

		constexpr Iterator& operator++() noexcept(noexcept(++n)) {
			++n;
			return *this;
		}

		constexpr Iterator operator++(int) noexcept(noexcept(++n) && noexcept(Iterator(*this))) {
			auto result(*this);
			++n;
			return result;
		}

		constexpr Iterator& operator--() noexcept(noexcept(--n)) {
			--n;
			return *this;
		}

		constexpr Iterator operator--(int) noexcept(noexcept(--n) && noexcept(Iterator(*this))) {
			auto result(*this);
			--n;
			return result;
		}

		constexpr T const& operator*() const noexcept { return n; }
		constexpr T const* operator->() const noexcept { return &n; }

		// Well, we cannot assume c++20....
		constexpr bool operator==(Iterator const& other) noexcept(noexcept(n == n)) { return n == other.n; }
		constexpr bool operator!=(Iterator const& other) noexcept(noexcept(n != n)) { return n != other.n; }
		constexpr bool operator<(Iterator const& other) noexcept(noexcept(n < n)) { return n < other.n; }
		constexpr bool operator<=(Iterator const& other) noexcept(noexcept(n <= n)) { return n <= other.n; }
		constexpr bool operator>(Iterator const& other) noexcept(noexcept(n > n)) { return n > other.n; }
		constexpr bool operator>=(Iterator const& other) noexcept(noexcept(n >= n)) { return n >= other.n; }
	};

	constexpr Iterator begin() const { return {first}; }
	constexpr Iterator end() const {
		Iterator result{last};
		++result;
		assert(!(*result < last));
		return result;
	}
};

template <typename T>
inline constexpr auto make_inclusive_number_range(T first, T last) noexcept(std::is_trivially_move_constructible_v<T>) {
	return InclusiveNumberRange<T>{std::move(first), std::move(last)};
}

template <typename T> struct fmt::formatter<InclusiveNumberRange<T>> {
	template <typename ParseContext> constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

	template <typename FormatContext> auto format(InclusiveNumberRange<T> const& range, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "[{}, {}]", range.first, range.last);
	}
};
