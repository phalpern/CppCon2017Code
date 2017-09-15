/* pmr_string.h                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_PMR_STRING_DOT_H
#define INCLUDED_PMR_STRING_DOT_H

#include <string>
#include <polymorphic_allocator.h>

namespace cpp17 {
namespace pmr {

    // C++17 string that uses a polymorphic allocator
    template <class CharT, class traits = char_traits<CharT>>
    using basic_string = std::basic_string<CharT, traits,
                                           polymorphic_allocator<CharT>>;

    using string  = basic_string<char>;
    using wstring = basic_string<wchar_t>;
}
}

#endif // ! defined(INCLUDED_PMR_STRING_DOT_H)
