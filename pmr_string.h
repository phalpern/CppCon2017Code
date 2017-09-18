/* pmr_string.h                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_PMR_STRING_DOT_H
#define INCLUDED_PMR_STRING_DOT_H

#include <type_traits>
#include <string>
#include <polymorphic_allocator.h>

namespace cpp17 {
namespace pmr {

// Does this implementantation have C++11-compliant strings with extended copy
// constructor and move constructor?  Assume yes unless we know that our
// (otherwise C++11-compliant compiler) has a broken library.
#if defined(__GNUC__)
# if (__GNUC__ > 6)
    // gcc 6.3 and before does not have a compliant string class library.  It
    // is missing the copy-construct-with-allocator and
    // move-constructor-with-allocator.
#   define CPP11_COMPLIANT_STRING 1
# endif
#else
# define CPP11_COMPLIANT_STRING 1
#endif

#ifdef CPP11_COMPLIANT_STRING

    // C++17 string that uses a polymorphic allocator
    template <class charT, class traits = char_traits<charT>>
    using basic_string = std::basic_string<charT, traits,
                                           polymorphic_allocator<charT>>;

    using string  = basic_string<char>;
    using wstring = basic_string<wchar_t>;

#else // if not C++11-compliant string library

    // The string library in gcc 6.3 doesn't support extended copy and move
    // constructors (i.e., `basic_string(const basic_string&, const
    // Allocator&)` and `basic_string(basic_string&&, const Allocator&)`, so
    // we have to fake it here in order to support scoped allocator
    // operations for things like `pmr::vector<pmr::string>`.
    template <class charT, class traits = char_traits<charT>>
    class basic_string
        : public std::basic_string<charT, traits, polymorphic_allocator<charT>>
    {
        using Allocator = polymorphic_allocator<charT>;
        using Base = std::basic_string<charT, traits, Allocator>;
        struct Dummy { };

      public:
        using size_type = typename Base::size_type;

        // Duplicate all constructors from `std::basic_string`
        explicit basic_string(const Allocator& a = Allocator()) : Base(a) {}
        basic_string(const basic_string& str)
            : Base(str.data(), str.size(), Allocator()) { }
        basic_string(basic_string&& str) noexcept
            : Base(std::move(str)) { }
        basic_string(const Base& str, size_type pos, size_type n = Base::npos,
                     const Allocator& a = Allocator())
            : Base(str, pos, n, a) {}
        basic_string(const charT* s,
                     size_type n, const Allocator& a = Allocator())
            : Base(s, n, a) {}
        basic_string(const charT* s, const Allocator& a = Allocator())
            : Base(s, a) {}
        basic_string(size_type n, charT c, const Allocator& a = Allocator())
            : Base(n, c, a) {}
        template<class InputIterator>
        basic_string(InputIterator begin, InputIterator end,
                     const Allocator& a = Allocator(),
                     typename enable_if<
                         is_constructible<Base,
                                          InputIterator, InputIterator>::value,
                         Dummy
                       >::type = {})
            : Base(begin, end, a) {}
        basic_string(initializer_list<charT> il,
                     const Allocator& a = Allocator())
            : Base(il, a) {}

        // Add the missing constructors
        basic_string(const Base& str, const Allocator& a)
            : Base(str.data(), str.size(), a) {}
        basic_string(Base&& str, const Allocator& a)
            : Base(a) {
            if (str.get_allocator() == a)
                *this = std::move(str); // Move
            else
                *this = str;            // copy
        }

        // And conversions from base class
        basic_string(const Base& str)
            : Base(str.data(), str.size(), Allocator()) { }
        basic_string(Base&& str) noexcept
            : Base(std::move(str)) { }

        // Forward assignment operators
        basic_string& operator=(const basic_string& str) {
            Base::operator=(str);
            return *this;
        }
        basic_string& operator=(const Base& str) {
            Base::operator=(str);
            return *this;
        }
        basic_string& operator=(basic_string&& str) {
            Base::operator=(std::move(str));
            return *this;
        }
        basic_string& operator=(Base&& str) {
            Base::operator=(std::move(str));
            return *this;
        }
        basic_string& operator=(const charT* s) {
            Base::operator=(s);
            return *this;
        }
        basic_string& operator=(charT* c) {
            Base::operator=(c);
            return *this;
        }
        basic_string& operator=(initializer_list<charT> il) {
            Base::operator=(il);
            return *this;
        }
    };

    using string  = basic_string<char>;
    using wstring = basic_string<wchar_t>;

#endif // Not C++11-compliant string library

} // Close namespace pmr
} // Close namespace cpp17

#endif // ! defined(INCLUDED_PMR_STRING_DOT_H)
