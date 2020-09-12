#pragma once
#include <string_view>
#include <utility>
#include <iostream>

namespace impl
{
/// Base declaration of our constexpr string_view concatenation helper
template <std::string_view const&, typename, std::string_view const&, typename>
struct concat;

/// Specialisation to yield indices for each char in both provided string_views,
/// allows us flatten them into a single char array
template <std::string_view const& S1,
          std::size_t... I1,
          std::string_view const& S2,
          std::size_t... I2>
struct concat<S1, std::index_sequence<I1...>, S2, std::index_sequence<I2...>>
{
  static constexpr const char value[]{S1[I1]..., S2[I2]..., 0};
};
} // namespace impl

/// Base definition for compile time joining of strings
template <std::string_view const&...> struct join;

/// When no strings are given, provide an empty literal
template <>
struct join<>
{
  static constexpr std::string_view value = "";
};

/// Base case for recursion where we reach a pair of strings, we concatenate
/// them to produce a new constexpr string
template <std::string_view const& S1, std::string_view const& S2>
struct join<S1, S2>
{
  static constexpr std::string_view value =
    impl::concat<S1,
                 std::make_index_sequence<S1.size()>,
                 S2,
                 std::make_index_sequence<S2.size()>>::value;
};

/// Main recursive definition for constexpr joining, pass the tail down to our
/// base case specialisation
template <std::string_view const& S, std::string_view const&... Rest>
struct join<S, Rest...>
{
  static constexpr std::string_view value =
    join<S, join<Rest...>::value>::value;
};

/// Join constexpr string_views to produce another constexpr string_view
template <std::string_view const&... Strs>
static constexpr auto join_v = join<Strs...>::value;
