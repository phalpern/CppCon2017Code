/* test_resource.cpp                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include "test_resource.h"
#include <algorithm>
#include <cassert>

// Keep track of number of bytes that would be leaked by
// allocator destructor.
size_t test_resource::s_leaked_bytes = 0;

// Keep track of number of blocks that would be leaked by
// allocator destructor.
size_t test_resource::s_leaked_blocks = 0;

test_resource::test_resource(pmr::memory_resource *parent)
  : m_parent(parent)
  , m_bytes_allocated(0)
  , m_bytes_outstanding(0)
  , m_bytes_highwater(0)
  , m_blocks(parent) // Vector memory comes from parent, too
{
}

test_resource::~test_resource() {
  // If any blocks have not been released, report them as leaked.
  s_leaked_blocks += blocks_outstanding();

  // Reclaim blocks that would have been leaked.
  for (auto& alloc_rec : m_blocks) {
    s_leaked_bytes += alloc_rec.m_bytes;
    m_parent->deallocate(alloc_rec.m_ptr, alloc_rec.m_bytes,
                         alloc_rec.m_alignment);
  }
}

pmr::memory_resource *test_resource::parent() const {
  return m_parent;
}

size_t test_resource::bytes_allocated() const {
  return m_bytes_allocated;
}

size_t test_resource::bytes_deallocated() const {
  return m_bytes_allocated - m_bytes_outstanding;
}

size_t test_resource::bytes_outstanding() const {
  return m_bytes_outstanding;
}

size_t test_resource::bytes_highwater() const {
  return m_bytes_highwater;
}

size_t test_resource::blocks_outstanding() const {
  return m_blocks.size();
}

size_t test_resource::leaked_bytes() {
  return s_leaked_bytes;
}

size_t test_resource::leaked_blocks() {
  return s_leaked_blocks;
}

void test_resource::clear_leaked() {
  s_leaked_bytes  = 0;
  s_leaked_blocks = 0;
}

void *test_resource::do_allocate(size_t bytes,
                                 size_t alignment) {

  void *ret = m_parent->allocate(bytes, alignment);
  m_blocks.push_back(allocation_rec{ret, bytes, alignment});
  m_bytes_allocated   += bytes;
  m_bytes_outstanding += bytes;
  if (m_bytes_outstanding > m_bytes_highwater)
    m_bytes_highwater = m_bytes_outstanding;

  return ret;
}

void test_resource::do_deallocate(void *p, size_t bytes,
                                  size_t alignment) {
  // Check that deallocation args exactly match allocation args.
  auto i = std::find_if(m_blocks.begin(), m_blocks.end(),
                        [p](allocation_rec& r){
                          return r.m_ptr == p; });
  if (i == m_blocks.end())
    throw std::invalid_argument("deallocate: Invalid pointer");
  else if (i->m_bytes != bytes)
    throw std::invalid_argument("deallocate: Size mismatch");
  else if (i->m_alignment != alignment)
    throw std::invalid_argument("deallocate: Alignment mismatch");

  m_parent->deallocate(p, i->m_bytes, i->m_alignment);
  m_blocks.erase(i);
  m_bytes_outstanding -= bytes;
}

bool test_resource::do_is_equal(const pmr::memory_resource& other)
                                     const noexcept {
  return this == &other;
}

/* End test_resource.cpp */

// For best result when pasting into PowerPoint, set indent to 2
// and right-most column to 64.
//
// Local Variables:
// c-basic-offset: 2
// fill-column: 64
// End:
