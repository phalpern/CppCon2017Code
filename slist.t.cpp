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
    poly_alloc    ta(&tr);

    std::cout << "Testing basic constructor\n";
    {
        slist<int> lst1;
        ASSERT(lst1.empty());
        ASSERT(0 == lst1.size());
        ASSERT(lst1.begin() == lst1.end());
        ASSERT(poly_alloc{} == lst1.get_allocator());

        slist<int> lst2(&tr);  // auto-convert resource ptr to poly_alloc
        ASSERT(ta == lst2.get_allocator());
        ASSERT(0  == tr.blocks_outstanding());
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
        ASSERT(ta == lst4.front().get_allocator());
        ASSERT(check(lst4, { "10" }, &tr));
        lst4.front().resize(33);  // Exceed small-object optimization
        ASSERT(2 == tr.blocks_outstanding());
        lst4.front().resize(2);
        lst4.emplace_back(2, '1');  // '1' x 2 -> "11"
        ASSERT("10" == lst4.front());
        ASSERT(check(lst4, { "10", "11" }, &tr));
        lst4.emplace_front("1zzq", 1);
        ASSERT("1" == lst4.front());
        ASSERT(check(lst4, { "1", "10", "11" }, &tr));

        static const char sixes[] = "six six six six 6 six six six six";
        auto i = ++lst4.begin();
        ASSERT("10" == *i);
        *i++ = sixes;
        ASSERT(check(lst4, { "1", sixes, "11" }));
        ASSERT("11" == *i);

        i = lst4.emplace(i, 1, '7');
        ASSERT("7" == *i);
        ASSERT(check(lst4, { "1", sixes, "7", "11" }));
        ++i;
        i = lst4.emplace(i, "nine\n", 4);
        ASSERT("nine" == *i);
        ASSERT(check(lst4, { "1", sixes, "7", "nine", "11" }));
        i = lst4.insert(i, pmr::string("8"));
        ASSERT(check(lst4, { "1", sixes, "7", "8", "nine", "11" }));

        std::cout << "Testing erase()\n";
        auto pre_erase_blks = tr.blocks_outstanding();
        i = lst4.erase(i, std::next(i, 2));
        ASSERT(check(lst4, { "1", sixes, "7", "11" }));
        ASSERT(tr.blocks_outstanding() <= pre_erase_blks - 2);
        ASSERT("11" == *i);
        pre_erase_blks = tr.blocks_outstanding();
        i = lst4.erase(i);
        ASSERT(check(lst4, { "1", sixes, "7" }));
        ASSERT(tr.blocks_outstanding() < pre_erase_blks);
        ASSERT(lst4.end() == i);
        pre_erase_blks = tr.blocks_outstanding();
        i = lst4.erase(lst4.begin());
        ASSERT(check(lst4, { sixes, "7" }));
        ASSERT(tr.blocks_outstanding() < pre_erase_blks);
        ASSERT(lst4.begin() == i);
        ASSERT(sixes == *i);

        std::cout << "Testing copy constructor and equality operators\n";
        {
            auto pre_copy_blks = tr.blocks_outstanding();
            slist<pmr::string> lst5(lst4);
            ASSERT(lst5 == lst4);
            ASSERT(! (lst5 != lst4));
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst5, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == pre_copy_blks);
            ASSERT(lst5.get_allocator().resource() ==
                   pmr::new_delete_resource_singleton());
            lst5.push_back("12");
            ASSERT(lst5 != lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst5, { sixes, "7", "12" }));
        }

        std::cout << "Testing extended copy constructor\n";
        {
            test_resource tr2;
            pmr::polymorphic_allocator<cpp17::byte> ta2(&tr2);
            auto pre_copy_blks = tr.blocks_outstanding();
            slist<pmr::string> lst6(lst4, &tr2);
            ASSERT(lst6 == lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst6, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == pre_copy_blks);
            ASSERT(tr2.blocks_outstanding() == tr.blocks_outstanding());
            ASSERT(lst6.get_allocator() == ta2);
            lst6.front() = "5";
            ASSERT(lst6 != lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst6, { "5", "7" }));
        }

        std::cout << "Testing move constructor\n";
        {
            test_resource tr2;

            slist<pmr::string> lst4b(lst4, &tr2);  // Copy
            auto pre_move_blks = tr2.blocks_outstanding();
            slist<pmr::string> lst7(std::move(lst4b)); // Move from copy
            ASSERT(lst7 == lst4);
            ASSERT(lst4b.empty())
            ASSERT(check(lst7, { sixes, "7" }));
            ASSERT(tr2.blocks_outstanding() == pre_move_blks); // no alloc/free
            ASSERT(lst7.get_allocator().resource() == &tr2)
            lst7.push_back("12");
            ASSERT(lst7 != lst4);
            ASSERT(check(lst7, { sixes, "7", "12" }));
        }

        std::cout << "Testing extended move constructor\n";
        {
            slist<pmr::string> lst4b(lst4, &tr);  // Copy
            // Extended move to the same allocator works just like a move
            auto pre_move_blks = tr.blocks_outstanding();
            slist<pmr::string> lst8(std::move(lst4b), &tr);
            ASSERT(lst8 == lst4);
            ASSERT(lst4b.empty())
            ASSERT(check(lst8, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == pre_move_blks); // no alloc/free
            ASSERT(lst8.get_allocator().resource() == &tr)
            lst8.push_back("12");
            ASSERT(lst8 != lst4);
            ASSERT(check(lst8, { sixes, "7", "12" }));
        }
        {
            test_resource tr2;
            pmr::polymorphic_allocator<cpp17::byte> ta2(&tr2);

            // Extended move to a new allocator works just like a copy
            auto pre_move_blks = tr.blocks_outstanding();
            slist<pmr::string> lst9(std::move(lst4), &tr2);
            ASSERT(lst9 == lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst9, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == pre_move_blks);
            ASSERT(tr2.blocks_outstanding() == tr.blocks_outstanding());
            ASSERT(lst9.get_allocator() == ta2);
            lst9.front() = "5";
            ASSERT(lst9 != lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst9, { "5", "7" }));
        }

        std::cout << "Testing copy assignment\n";
        {
            auto pre_copy_blks = tr.blocks_outstanding();
            slist<pmr::string> lst10(&tr);
            lst10.push_back("stuff");
            lst10 = lst4;
            ASSERT(lst10 == lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst10, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == 2 * pre_copy_blks);
            lst10.push_back("12");
            ASSERT(lst10 != lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst10, { sixes, "7", "12" }));
        }

        std::cout << "Testing move assignment\n";
        {
            test_resource tr2;
            slist<pmr::string> lst4b(lst4, &tr2);  // Copy

            // move-assign with the same allocator
            auto pre_move_blks = tr2.blocks_outstanding();
            slist<pmr::string> lst11(lst4b.get_allocator());
            lst11.push_back("stuff");
            lst11 = std::move(lst4b);
            ASSERT(lst11 == lst4);
            ASSERT(lst4b.empty());
            ASSERT(check(lst11, { sixes, "7" }));
            ASSERT(tr2.blocks_outstanding() == pre_move_blks); // no alloc/free
            lst11.push_back("12");
            ASSERT(lst11 != lst4);
            ASSERT(check(lst11, { sixes, "7", "12" }));
        }
        {
            test_resource tr2;

            // move-assign with different allocator is like copy-asign
            auto pre_move_blks = tr.blocks_outstanding();
            slist<pmr::string> lst12(&tr2);
            lst12.push_front("stuff");
            lst12 = std::move(lst4);
            ASSERT(lst12 == lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst12, { sixes, "7" }));
            ASSERT(tr.blocks_outstanding() == pre_move_blks);
            ASSERT(tr2.blocks_outstanding() == tr.blocks_outstanding());
            lst12.front() = "5";
            ASSERT(lst12 != lst4);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst12, { "5", "7" }));
        }

        std::cout << "Testing swap()\n";
        {
            slist<pmr::string> lst13(&tr);
            lst13.push_front("hello");
            lst13.push_back("world");
            ASSERT(check(lst13, { "hello", "world" }));
            auto pre_swap_blks = tr.blocks_outstanding();
            lst4.swap(lst13);
            ASSERT(tr.blocks_outstanding() == pre_swap_blks);
            using namespace std;
            swap(lst4, lst13);
            ASSERT(check(lst4, { sixes, "7" }));
            ASSERT(check(lst13, { "hello", "world" }));
            ASSERT(tr.blocks_outstanding() == pre_swap_blks);
        }
    }
    ASSERT(0 == tr.blocks_outstanding());  // No leaks

    printf("%s\n", 0 == testStatus ? "PASSED" : "FAILED");

    return testStatus;
}

/* End slist.t.cpp */
