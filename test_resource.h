/* test_resource.h                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_TEST_RESOURCE_DOT_H
#define INCLUDED_TEST_RESOURCE_DOT_H

#include <polymorphic_allocator.h>
#include <pmr_vector.h>

using std::size_t;
namespace pmr = cpp17::pmr;

class test_resource : public pmr::memory_resource
{
public:
  explicit test_resource(pmr::memory_resource *parent =
                           pmr::get_default_resource());
  ~test_resource();

  pmr::memory_resource *parent() const;

  size_t bytes_allocated() const;
  size_t bytes_deallocated() const;
  size_t bytes_outstanding() const;
  size_t bytes_highwater() const;
  size_t blocks_outstanding() const;

  static size_t leaked_bytes();
  static size_t leaked_blocks();
  static void   clear_leaked();

protected:
  void *do_allocate(size_t bytes, size_t alignment) override;
  void do_deallocate(void *p, size_t bytes,
                     size_t alignment) override;
  bool do_is_equal(const pmr::memory_resource& other)
                                        const noexcept override;

private:
  // Record holding the results of an allocation
  struct allocation_rec {
    void   *m_ptr;
    size_t  m_bytes;
    size_t  m_alignment;
  };

  pmr::memory_resource        *m_parent;
  size_t                       m_bytes_allocated;
  size_t                       m_bytes_outstanding;
  size_t                       m_bytes_highwater;
  pmr::vector<allocation_rec>  m_blocks;

  static size_t                s_leaked_bytes;
  static size_t                s_leaked_blocks;
};

#endif // ! defined(INCLUDED_TEST_RESOURCE_DOT_H)

// For best result when pasting into PowerPoint, set indent to 2
// and right-most column to 64.
//
// Local Variables:
// c-basic-offset: 2
// fill-column: 64
// End:
