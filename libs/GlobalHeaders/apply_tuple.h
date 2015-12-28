#pragma once
#include <cstddef> // for std::size_t
namespace apply_tuple
{
// Define holder for indices...
template <std::size_t...>
struct indices;

// Define adding the Nth index...
// Notice one argument, Type, is discarded each recursive call.
// Notice N is added to the end of Indices... .
// Notice N is incremented with the recursion.
template <std::size_t N, typename Indices, typename... Types>
struct make_indices_impl;

template <
  std::size_t N,
  std::size_t... Indices,
  typename Type,
  typename... Types
>
struct make_indices_impl<N, indices<Indices...>, Type, Types...>
{
  typedef
    typename make_indices_impl<
      N+1,
      indices<Indices...,N>,
      Types...
    >::type
    type
  ;
};

// Define adding the last index...
// Notice no Type or Types... are left.
// Notice the full solution is emitted into the container.
template <std::size_t N, std::size_t... Indices>
struct make_indices_impl<N, indices<Indices...>>
{
  typedef indices<Indices...> type;
};

// Compute the indices starting from zero...
// Notice indices container is empty.
// Notice Types... will be all of the tuple element types.
// Notice this refers to the full solution (i.e., via ::type).
template <std::size_t N, typename... Types>
struct make_indices
{
  typedef
    typename make_indices_impl<0, indices<>, Types...>::type
    type
  ;
};

template <typename Indices>
struct apply_tuple_impl;

template <
  template <std::size_t...> class I,
  std::size_t... Indices
>
struct apply_tuple_impl<I<Indices...>>
{
  template <
    typename Op,
    typename... OpArgs,
    template <typename...> class T = std::tuple
  >
  static auto apply_tuple(Op op, T<OpArgs...> t)
    -> typename std::result_of<Op(OpArgs...)>::type
  {
    return op( std::forward<OpArgs>(std::get<Indices>(t))... );
  }
};

template <
  typename Op,
  typename... OpArgs,
  typename Indices = typename make_indices<0, OpArgs...>::type,
  template <typename...> class T = std::tuple
>
auto apply(Op op, T<OpArgs...> t)
  -> typename std::result_of<Op(OpArgs...)>::type
{
  return apply_tuple_impl<Indices>::apply_tuple(
    std::forward<Op>(op),
    std::forward<T<OpArgs...>>(t)
  );
}



}
