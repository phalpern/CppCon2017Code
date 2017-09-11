/* make_from_tuple.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_MAKE_FROM_TUPLE_DOT_H
#define INCLUDED_MAKE_FROM_TUPLE_DOT_H

#include <utility>
#include <tuple>
#include <cstdlib>
#include <integer_sequence.h>

namespace cpp17 {

using namespace std;
using namespace cpp14;

template <class T> using decay_t = typename decay<T>::type;

template <class...> struct void_t_imp { typedef void type; };
template <class... T> using void_t = typename void_t_imp<T...>::type;

namespace internal {

template <class F, class Tuple, size_t... I>
constexpr auto apply_impl(F&& f, Tuple&& t, index_sequence<I...>) ->
    decltype(std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...))
{
    // TBD: This should be a call to `invoke()`
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

template <class T, class Tuple, size_t... Indexes>
T make_from_tuple_imp(Tuple&& t, index_sequence<Indexes...>)
{
    return T(get<Indexes>(forward<Tuple>(t))...);
}

#if 0
template <class T, class Tuple, size_t... Indexes>
T* uninitialized_construct_from_tuple_imp(T* p, Tuple&& t,
                                          index_sequence<Indexes...>)
{
    return ::new((void*) p) T(get<Indexes>(std::forward<Tuple>(t))...);
}
#endif

} // close namespace namespace cpp17::internal

template <class F, class Tuple>
constexpr auto apply(F&& f, Tuple&& t) ->
    decltype(internal::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                      make_index_sequence<tuple_size<decay_t<Tuple>>::value>{}))
{
    return internal::apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                    make_index_sequence<tuple_size<decay_t<Tuple>>::value>{});
}

template <class T, class Tuple>
T make_from_tuple(Tuple&& args_tuple)
{
    using namespace internal;
    using Indices = make_index_sequence<tuple_size<decay_t<Tuple>>::value>;
    return make_from_tuple_imp<T>(forward<Tuple>(args_tuple), Indices{});
}

#if 0
template <class T, class Tuple>
T* uninitialized_construct_from_tuple(T* p, Tuple&& args_tuple)
{
    using namespace internal;
    using Indices = make_index_sequence<tuple_size<decay_t<Tuple>>::value>;
    return uninitialized_construct_from_tuple_imp<T>(p,
                                                     forward<Tuple>(args_tuple),
                                                     Indices{});
}
#endif

} // close namespace cpp17

#endif // ! defined(INCLUDED_MAKE_FROM_TUPLE_DOT_H)
