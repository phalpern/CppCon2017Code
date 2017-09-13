/* test_resource.t.cpp                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include "test_resource.h"

#include <iostream>
#include <cstdlib>
#include <climits>
#include <cstring>

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


int main(int argc, char *argv[])
{
    using namespace cpp17::pmr;

    std::cout << "Testing constructors\n";
    test_resource tr1;
    ASSERT(0 == tr1.bytes_allocated());
    ASSERT(0 == tr1.bytes_deallocated());
    ASSERT(0 == tr1.bytes_outstanding());
    ASSERT(0 == tr1.bytes_highwater());
    ASSERT(0 == tr1.blocks_outstanding());
    ASSERT(new_delete_resource_singleton() == tr1.parent());
    test_resource tr2(&tr1);
    ASSERT(&tr1 == tr2.parent());

    std::cout << "Testing operator ==\n";
    ASSERT(tr1.is_equal(tr1));
    ASSERT(tr1 == tr1);
    ASSERT(! tr1.is_equal(tr2));
    ASSERT(tr1 != tr2);

    std::cout << "Testing statistics gathering\n";
    void* p1 = tr1.allocate(40);
    ASSERT(40 == tr1.bytes_allocated());
    ASSERT(0  == tr1.bytes_deallocated());
    ASSERT(40 == tr1.bytes_outstanding());
    ASSERT(40 == tr1.bytes_highwater());
    ASSERT(1  == tr1.blocks_outstanding());
    tr1.deallocate(p1, 40);
    ASSERT(40 == tr1.bytes_allocated());
    ASSERT(40 == tr1.bytes_deallocated());
    ASSERT(0  == tr1.bytes_outstanding());
    ASSERT(40 == tr1.bytes_highwater());
    ASSERT(0  == tr1.blocks_outstanding());
    p1 = tr1.allocate(32);
    ASSERT(72 == tr1.bytes_allocated());
    ASSERT(40 == tr1.bytes_deallocated());
    ASSERT(32 == tr1.bytes_outstanding());
    ASSERT(40 == tr1.bytes_highwater());
    ASSERT(1  == tr1.blocks_outstanding());
    void *p2 = tr1.allocate(64, 2);
    ASSERT(136 == tr1.bytes_allocated());
    ASSERT(40  == tr1.bytes_deallocated());
    ASSERT(96  == tr1.bytes_outstanding());
    ASSERT(96  == tr1.bytes_highwater());
    ASSERT(2   == tr1.blocks_outstanding());
    tr1.deallocate(p1, 32);
    ASSERT(136 == tr1.bytes_allocated());
    ASSERT(72  == tr1.bytes_deallocated());
    ASSERT(64  == tr1.bytes_outstanding());
    ASSERT(96  == tr1.bytes_highwater());
    ASSERT(1   == tr1.blocks_outstanding());

    std::cout << "Testing detection of mismatched pointer on deallocation\n";
    try {
        int dummy = 0;
        tr1.deallocate(&dummy, sizeof(int), sizeof(int));
        ASSERT(false && "unreachable");
    } catch (std::invalid_argument& ex) {
        const char *experr = "Invalid pointer passed to deallocate()";
        LOOP_ASSERT(ex.what(), 0 == strcmp(experr, ex.what()));
    }

    std::cout << "Testing detection of mismatched size on deallocation\n";
    try {
        tr1.deallocate(p2, 32, 2);
        ASSERT(false && "unreachable");
    } catch (std::invalid_argument& ex) {
        const char *experr = "Block-size mismatch on deallocate()";
        LOOP_ASSERT(ex.what(), 0 == strcmp(experr, ex.what()));
    }

    std::cout << "Testing detection of mismatched alignment on deallocation\n";
    try {
        tr1.deallocate(p2, 64, 4);
        ASSERT(false && "unreachable");
    } catch (std::invalid_argument& ex) {
        const char *experr = "Alignment mismatch on deallocate()";
        LOOP_ASSERT(ex.what(), 0 == strcmp(experr, ex.what()));
    }

    tr1.deallocate(p2, 64, 2);
    ASSERT(136 == tr1.bytes_allocated());
    ASSERT(136 == tr1.bytes_deallocated());
    ASSERT(0   == tr1.bytes_outstanding());
    ASSERT(96  == tr1.bytes_highwater());
    ASSERT(0   == tr1.blocks_outstanding());

    std::cout << "Testing delegation to parent resource\n";
    test_resource parentr;
    {
        test_resource tr3(&parentr);
        ASSERT(&parentr == tr3.parent());
        ASSERT(0 == parentr.blocks_outstanding());

        // First allocation in tr3 allocates an extra block in parent for
        // internal vector.
        void *p3 = tr3.allocate(128);  (void) p3;
        ASSERT(1 == tr3.blocks_outstanding());
        ASSERT(2 == parentr.blocks_outstanding());

        void *p4 = tr3.allocate(18, 2);
        void *p5 = tr3.allocate(8, 8); (void) p5;
        ASSERT(154 == tr3.bytes_outstanding());
        ASSERT(3   == tr3.blocks_outstanding());
        ASSERT(4   == parentr.blocks_outstanding());

        // No leak recorded until resource is destroyed.
        ASSERT(0 == test_resource::leaked_bytes());
        ASSERT(0 == test_resource::leaked_blocks());

        tr3.deallocate(p4, 18, 2);
        tr3.deallocate(p5, 8, 8);
        tr3.deallocate(p3, 128);
        ASSERT(0 == tr3.bytes_outstanding());
        ASSERT(0 == tr3.blocks_outstanding());
        ASSERT(1 >= parentr.blocks_outstanding());

        // tr3 destroyed -- no leak
    }
    ASSERT(0 == test_resource::leaked_bytes());
    ASSERT(0 == test_resource::leaked_blocks());
    ASSERT(0 == parentr.blocks_outstanding());

    std::cout << "Testing detection of leaks on resource destruction\n";
    {
        test_resource tr4(&parentr);
        ASSERT(&parentr == tr4.parent());
        ASSERT(0 == parentr.blocks_outstanding());

        void *p3 = tr4.allocate(128);   (void) p3;
        void *p4 = tr4.allocate(18, 2);
        void *p5 = tr4.allocate(8, 8);  (void) p5;

        ASSERT(154 == tr4.bytes_outstanding());
        ASSERT(3   == tr4.blocks_outstanding());
        ASSERT(4   == parentr.blocks_outstanding());

        tr4.deallocate(p4, 18, 2);
        ASSERT(136 == tr4.bytes_outstanding());
        ASSERT(2   == tr4.blocks_outstanding());
        ASSERT(3   == parentr.blocks_outstanding());

        // No leak recorded until resource is destroyed.
        ASSERT(0 == test_resource::leaked_bytes());
        ASSERT(0 == test_resource::leaked_blocks());

        // tr4 destructor called here, leaking 2 blocks.
    }
    LOOP_ASSERT(test_resource::leaked_bytes(),
                136 == test_resource::leaked_bytes());
    LOOP_ASSERT(test_resource::leaked_blocks(),
                2 == test_resource::leaked_blocks());
    // Test that all "leaked" blocks were actually returned to parent resource
    LOOP_ASSERT(parentr.blocks_outstanding(),
                0 == parentr.blocks_outstanding());

    test_resource::clear_leaked();
    ASSERT(0 == test_resource::leaked_bytes());
    ASSERT(0 == test_resource::leaked_blocks());

    std::cout << (0 == testStatus ? "PASSED" : "FAILED") << std::endl;

    return testStatus;
}

/* End test_resource.t.cpp */
