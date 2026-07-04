// CyberMeshGenerator — cmg::expected.
//
// Aliases std::expected when the standard library provides it (C++23), otherwise
// falls back to a minimal, dependency-free implementation covering the subset the
// mesher uses: value-or-error, has_value()/operator bool, value(), error(),
// value_or(), and construction from cmg::unexpected. This lets the public API use
// the modern `expected` shape while still building on a strict C++20 toolchain.
#pragma once

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#include <expected>
namespace cmg {
template <class T, class E> using expected = std::expected<T, E>;
template <class E> using unexpected = std::unexpected<E>;
} // namespace cmg
#else

#include <type_traits>
#include <utility>
#include <variant>

namespace cmg {

/// Wraps an error value to disambiguate the error alternative of `expected`.
template <class E>
class unexpected {
public:
    explicit unexpected(E e) : error_(std::move(e)) {}
    const E& error() const& noexcept { return error_; }
    E& error() & noexcept { return error_; }
    E&& error() && noexcept { return std::move(error_); }

private:
    E error_;
};

/// Minimal std::expected<T, E> stand-in for C++20 toolchains.
template <class T, class E>
class expected {
public:
    using value_type = T;
    using error_type = E;

    expected() : store_(std::in_place_index<0>, T{}) {}
    expected(const T& v) : store_(std::in_place_index<0>, v) {}
    expected(T&& v) : store_(std::in_place_index<0>, std::move(v)) {}

    expected(const unexpected<E>& u) : store_(std::in_place_index<1>, u.error()) {}
    expected(unexpected<E>&& u)
        : store_(std::in_place_index<1>, std::move(u).error()) {}

    bool has_value() const noexcept { return store_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    T& value() & { return std::get<0>(store_); }
    const T& value() const& { return std::get<0>(store_); }
    T&& value() && { return std::get<0>(std::move(store_)); }

    T& operator*() & noexcept { return std::get<0>(store_); }
    const T& operator*() const& noexcept { return std::get<0>(store_); }
    T* operator->() noexcept { return &std::get<0>(store_); }
    const T* operator->() const noexcept { return &std::get<0>(store_); }

    E& error() & { return std::get<1>(store_); }
    const E& error() const& { return std::get<1>(store_); }
    E&& error() && { return std::get<1>(std::move(store_)); }

    template <class U>
    T value_or(U&& fallback) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(fallback));
    }

private:
    std::variant<T, E> store_;
};

} // namespace cmg
#endif
