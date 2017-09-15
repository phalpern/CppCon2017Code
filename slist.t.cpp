/* slist.t.cpp                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include "slist.h"
#include <polymorphic_allocator.h>
#include <pmr_string.h>
#include <test_resource.h>

#include <iostream>
#include <initializer_list>

//==========================================================================
//                  ASSERT TEST MACRO
//--------------------------------------------------------------------------
static int testStatus = 0;

static void aSsErT(int c, const char *s, int i) {
    if (c) {
        std::cout << __FILE__ << ":" << i << ": error: " << s
                  << "    (failed)" << std::endl;
        if (testStatus >= 0 && testStatus <= 100) ++testStatus;
    }
}

# define ASSERT(X) { aSsErT(!(X), #X, __LINE__); }
//--------------------------------------------------------------------------
#define LOOP_ASSERT(I,X) { \
    if (!(X)) { std::cout << #I << ": " << I << "\n"; \
                aSsErT(1, #X, __LINE__); } }

#define LOOP2_ASSERT(I,J,X) { \
    if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " \
                          << J << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP3_ASSERT(I,J,K,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J \
                         << "\t" << #K << ": " << K << "\n";           \
                aSsErT(1, #X, __LINE__); } }

#define LOOP4_ASSERT(I,J,K,L,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J \
                         << "\t" << #K << ": " << K << "\t" << #L << ": " \
                         << L << "\n"; aSsErT(1, #X, __LINE__); } }

#define LOOP5_ASSERT(I,J,K,L,M,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J    \
                         << "\t" << #K << ": " << K << "\t" << #L << ": " \
                         << L << "\t" << #M << ": " << M << "\n";         \
               aSsErT(1, #X, __LINE__); } }

#define LOOP6_ASSERT(I,J,K,L,M,N,X) { \
   if (!(X)) { std::cout << #I << ": " << I << "\t" << #J << ": " << J     \
                         << "\t" << #K << ": " << K << "\t" << #L << ": "  \
                         << L << "\t" << #M << ": " << M << "\t" << #N     \
                         << ": " << N << "\n"; aSsErT(1, #X, __LINE__); } }

// Check the integrity and value of the specified container.
template <typename Container>
bool check(const Container& c,
           std::initializer_list<typename Container::value_type> v)
{
    // Validate that size is the same as iterator range length.
    LOOP2_ASSERT(c.size(), std::distance(c.begin(), c.end()),
                 c.size() == (size_t) std::distance(c.begin(), c.end()))

    if (c.size() == v.size())
        return std::equal(c.begin(), c.end(), v.begin());
    else
        return false;
}

// Check the integrity and value of the specified container, including that
// every member uses the same allocator.
template <typename Container>
bool check(const Container& c,
           std::initializer_list<typename Container::value_type> v,
           pmr::polymorphic_allocator<cpp17::byte> a)
{
    if (check(c, v)) {
        ASSERT(a == c.get_allocator());
        for (auto& element : c)
            ASSERT(a == element.get_allocator());
        return true;
    }
    else
        return false;
}

int main(int argc, char *argv[])
{
    using namespace cpp17::pmr;
    using cpp17::byte;
    using poly_alloc = pmr::polymorphic_allocator<byte>;

    test_resource tr;

    std::cout << "Testing basic constructor\n";
    {
        slist<int> lst1;
        ASSERT(lst1.empty());
        ASSERT(0 == lst1.size());
        ASSERT(lst1.begin() == lst1.end());
        ASSERT(poly_alloc{} == lst1.get_allocator());

        slist<int> lst2(&tr);  // auto-convert resource ptr to poly_alloc
        ASSERT(poly_alloc(&tr) == lst2.get_allocator());
        ASSERT(0 == tr.blocks_outstanding());
    }

    {
        std::cout << "Testing push_back & push_front\n";
        slist<int> lst3(&tr);
        lst3.push_back(10);
        ASSERT(10 == lst3.front());
        ASSERT(check(lst3, { 10 }));
        ASSERT(lst3.size() == tr.blocks_outstanding());
        lst3.push_back(11);
        ASSERT(10 == lst3.front());
        ASSERT(check(lst3, { 10, 11 }));
        ASSERT(lst3.size() == tr.blocks_outstanding());
        lst3.push_front(1);
        ASSERT(1 == lst3.front());
        ASSERT(check(lst3, { 1, 10, 11 }));
        ASSERT(lst3.size() == tr.blocks_outstanding());

        std::cout << "Testing simple iterator functionality\n";
        auto i = lst3.begin();
        ASSERT(1 == *i);
        ++i;
        ASSERT(10 == *i);
        *i = 5;
        ASSERT(check(lst3, { 1, 5, 11 }));
        *i++ = 6;
        ASSERT(check(lst3, { 1, 6, 11 }));
        ASSERT(11 == *i);

        i = lst3.insert(i, 7);
        ASSERT(7 == *i);
        ASSERT(check(lst3, { 1, 6, 7, 11 }));
        ASSERT(lst3.size() == tr.blocks_outstanding());
        ++i;
        i = lst3.insert(i, 9);
        ASSERT(9 == *i);
        ASSERT(check(lst3, { 1, 6, 7, 9, 11 }));
        ASSERT(lst3.size() == tr.blocks_outstanding());
        i = lst3.insert(i, 8);
        ASSERT(check(lst3, { 1, 6, 7, 8, 9, 11 }));
    }
    ASSERT(0 == tr.blocks_outstanding());  // No leaks

    std::cout << "Testing scoped allocator behavior:\n";
    {
        std::cout << "Testing emplace_back & emplace_front\n";
        slist<pmr::string> lst4(&tr);
        lst4.emplace_back("10", 2);
        ASSERT("10" == lst4.front());
        ASSERT(check(lst4, { "10" }, &tr));
        ASSERT(lst4.size() * 2 == tr.blocks_outstanding());
        lst4.emplace_back(2, '1');
        ASSERT("10" == lst4.front());
        ASSERT(check(lst4, { "10", "11" }, &tr));
        ASSERT(lst4.size() * 2 == tr.blocks_outstanding());
        lst4.emplace_front("1zzq", 1);
        ASSERT("1" == lst4.front());
        ASSERT(check(lst4, { "1", "10", "11" }, &tr));
        ASSERT(lst4.size() * 2 == tr.blocks_outstanding());

        lst4.pop_front();
    }
    ASSERT(0 == tr.blocks_outstanding());  // No leaks

    printf("%s\n", 0 == testStatus ? "PASSED" : "FAILED");

    return testStatus;
}

/* End slist.t.cpp */
