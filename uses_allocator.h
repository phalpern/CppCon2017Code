/* uses_allocator.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_USES_ALLOCATOR_DOT_H
#define INCLUDED_USES_ALLOCATOR_DOT_H

#include <make_from_tuple.h>
#include <utility>
#include <memory>

namespace cpp20 {

using namespace std;

////////////////////////////////////////////////////////////////////////

// Forward declaration
template <class T, class Alloc, class... Args>
auto uses_allocator_construction_args(const Alloc& a, Args&&... args);

namespace internal {

template <class T, class A>
struct has_allocator : std::uses_allocator<T, A> { };

// Specialization of `has_allocator` for `std::pair`
template <class T1, class T2, class A>
struct has_allocator<pair<T1, T2>, A>
    : integral_constant<bool, has_allocator<T1, A>::value ||
                              has_allocator<T2, A>::value>
{
};

template <bool V> using boolean_constant = integral_constant<bool, V>;

template <class T> struct is_pair : false_type { };

template <class T1, class T2>
struct is_pair<std::pair<T1, T2>> : true_type { };

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload is handles types for which `has_allocator<T, Alloc>` is false.
template <class T, class Unused1, class Unused2, class Alloc, class... Args>
auto uses_allocator_args_imp(Unused1      /* is_pair */,
                             false_type   /* has_allocator */,
                             Unused2      /* uses prefix allocator arg */,
                             const Alloc& /* ignored */,
                             Args&&... args)
{
    // Allocator is ignored
    return std::forward_as_tuple(std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles non-pair `T` for which `has_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` is valid.
template <class T, class Alloc, class... Args>
auto uses_allocator_args_imp(false_type /* is_pair */,
                             true_type  /* has_allocator */,
                             true_type  /* uses prefix allocator arg */,
                             const Alloc& a,
                             Args&&... args)
{
    // Allocator added to front of argument list, after `allocator_arg`.
    return std::forward_as_tuple(allocator_arg, a,
                                 std::forward<Args>(args)...);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles non-pair `T` for which `has_allocator<T, Alloc>` is
// true and constructor `T(allocator_arg_t, a, args...)` NOT valid.
// This function will produce invalid results unless `T(args..., a)` is valid.
template <class T1, class Alloc, class... Args>
auto uses_allocator_args_imp(false_type /* is_pair */,
                             true_type  /* has_allocator */,
                             false_type /* prefix allocator arg */,
                             const Alloc& a,
                             Args&&... args)
{
    // Allocator added to end of argument list
    return std::forward_as_tuple(std::forward<Args>(args)..., a);
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `T` = `std::pair` for which
// `has_allocator<T, Alloc>` is true for either or both of the elements and
// no other constructor arguments are passed in.
template <class T, class Alloc, class Tuple1, class Tuple2>
auto uses_allocator_args_imp(true_type  /* is_pair */,
                             true_type  /* has_allocator */,
                             false_type /* prefix allocator arg */,
                             const Alloc& a,
                             piecewise_construct_t,
                             Tuple1&& x, Tuple2&& y)
{
    using T1 = typename T::first_type;
    using T2 = typename T::second_type;

    return make_tuple(piecewise_construct,
                      apply([&a](auto&&... args1) -> auto {
                              return uses_allocator_construction_args<T1>(a,
                                     std::forward<decltype(args1)>(args1)...);
                          }, std::forward<Tuple1>(x)),
                      apply([&a](auto&&... args2) -> auto {
                              return uses_allocator_construction_args<T2>(a,
                                     std::forward<decltype(args2)>(args2)...);
                          }, std::forward<Tuple2>(y))
        );
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `T` = `std::pair` for which
// `has_allocator<T, Alloc>` is true for either or both of the elements and
// no other constructor arguments are passed in.
template <class T, class Alloc>
auto uses_allocator_args_imp(true_type  /* is_pair */,
                             true_type  /* has_allocator */,
                             false_type /* prefix allocator arg */,
                             const Alloc& a)
{
    // using T1 = typename T::first_type;
    // using T2 = typename T::second_type;

    // return std::make_tuple(
    //     piecewise_construct,
    //     uses_allocator_construction_args<T1>(a),
    //     uses_allocator_construction_args<T2>(a));
    return uses_allocator_construction_args<T>(a, piecewise_construct,
                                               tuple<>{}, tuple<>{});
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `T` = `std::pair` for which
// `has_allocator<T, Alloc>` is true for either or both of the elements and
// a single argument of type const-lvalue-of-pair is passed in.
template <class T, class Alloc, class U1, class U2>
auto uses_allocator_args_imp(true_type  /* is_pair */,
                                true_type  /* has_allocator */,
                                false_type /* prefix allocator arg */,
                                const Alloc& a,
                                const pair<U1, U2>& arg)
{
    // using T1 = typename T::first_type;
    // using T2 = typename T::second_type;

    // return std::make_tuple(
    //     piecewise_construct,
    //     uses_allocator_construction_args<T1>(a, arg.first),
    //     uses_allocator_construction_args<T2>(a, arg.second));
    return uses_allocator_construction_args<T>(a, piecewise_construct,
                                               forward_as_tuple(arg.first),
                                               forward_as_tuple(arg.second));
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `T` = `std::pair` for which
// `has_allocator<T, Alloc>` is true for either or both of the elements and
// a single argument of type rvalue-of-pair is passed in.
template <class T, class Alloc, class U1, class U2>
auto uses_allocator_args_imp(true_type  /* is_pair */,
                                true_type  /* has_allocator */,
                                false_type /* prefix allocator arg */,
                                const Alloc& a,
                                pair<U1, U2>&& arg)
{
    // using T1 = typename T::first_type;
    // using T2 = typename T::second_type;

    // return std::make_tuple(
    //     piecewise_construct,
    //     uses_allocator_construction_args<T1>(a, forward<U1>(arg.first)),
    //     uses_allocator_construction_args<T2>(a, forward<U2>(arg.second)));
    return uses_allocator_construction_args<T>(a, piecewise_construct,
                                   forward_as_tuple(forward<U1>(arg.first)),
                                   forward_as_tuple(forward<U2>(arg.second)));
}

// Return a tuple of arguments appropriate for uses-allocator construction
// with allocator `Alloc` and ctor arguments `Args`.
// This overload handles specializations of `T` = `std::pair` for which
// `has_allocator<T, Alloc>` is true for either or both of the elements and
// two additional constructor arguments are passed in.
template <class T, class Alloc, class U1, class U2>
auto uses_allocator_args_imp(true_type  /* is_pair */,
                             true_type  /* has_allocator */,
                             false_type /* prefix allocator arg */,
                             const Alloc& a,
                             U1&& arg1, U2&& arg2)
{
    // using T1 = typename T::first_type;
    // using T2 = typename T::second_type;

    // return std::make_tuple(
    //     piecewise_construct,
    //     uses_allocator_construction_args<T1>(a, forward<U1>(arg1)),
    //     uses_allocator_construction_args<T2>(a, forward<U2>(arg2)));
    return uses_allocator_construction_args<T>(a, piecewise_construct,
                                   forward_as_tuple(forward<U1>(arg1)),
                                   forward_as_tuple(forward<U2>(arg2)));
}

} // close namespace internal

template <class T, class Alloc, class... Args>
auto uses_allocator_construction_args(const Alloc& a, Args&&... args)
{
    using namespace internal;
    return uses_allocator_args_imp<T>(is_pair<T>(),
                                      has_allocator<T, Alloc>(),
                                      is_constructible<T, allocator_arg_t,
                                                       Alloc, Args...>(),
                                      a, std::forward<Args>(args)...);
}

template <class T, class Alloc, class... Args>
T make_using_allocator(const Alloc& a, Args&&... args)
{
    return make_from_tuple<T>(
        uses_allocator_construction_args<T>(a, forward<Args>(args)...));
}

template <class T, class Alloc, class... Args>
T* uninitialized_construct_using_allocator(T* p,
                                           const Alloc& a,
                                           Args&&... args)
{
    return apply([p](auto&&... args2){
            return ::new(static_cast<void*>(p))
                T(forward<decltype(args2)>(args2)...);
        }, uses_allocator_construction_args<T>(a, forward<Args>(args)...));
}

} // close namespace cpp20

#endif // ! defined(INCLUDED_USES_ALLOCATOR_DOT_H)
