/* slist.h                  -*-C++-*-
 *
 *            Copyright 2017 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_SLIST_DOT_H
#define INCLUDED_SLIST_DOT_H

#include <polymorphic_allocator.h>

namespace pmr = cpp17::pmr;

// Singly-linked list that supports the use of a polymorphic
// allocator.
template <typename Tp>
class slist {
public:
  using value_type     = Tp;
  using allocator_type = pmr::polymorphic_allocator<byte>;
  class iterator;
  class const_iterator;

  slist(allocator_type a = {});
  slist(const slist&, allocator_type a = {});
  slist(slist&&);
  slist(slist&&, allocator_type a = {});
  ~slist();

  slist& operator=(const slist&);
  slist& operator=(slist&&);
  void swap(slist& other);

  size_t size() const { return m_size; }
  bool   empty() const { return 0 == m_size; }

  iterator begin()              { return iterator(&m_head); }
  iterator end()                { return iterator(m_tail_p); }
  const_iterator begin() const  { return const_iterator(&m_head); }
  const_iterator end() const    { return const_iterator(&m_tail_p); }
  const_iterator cbegin() const { return const_iterator(&m_head); }
  const_iterator cend() const   { return const_iterator(m_tail_p); }

  Tp      & front()       { return m_head.m_next->m_value; }
  Tp const& front() const { return m_head.m_next->m_value; }

  template <typename... Args> void emplace(iterator i, Args&&... args);
  template <typename... Args> void emplace_front(Args&&... args)
    { emplace(begin(), std::forward<Args>(args)...); }
  template <typename... Args> void emplace_back(Args&&... args);
    { emplace(end(), std::forward<Args>(args)...); }

  void insert(iterator i, const Tp& v) { emplace(i, v); }
  void push_front(const Tp& v)         { emplace(begin(), v); }
  void push_back(const Tp& v)          { emplace(end(), v); }

  void erase(iterator i);
  void pop_front() { erase(begin()); }

  allocator_type get_allocator() const { return m_allocator; }

private:
  struct node;
  struct node_base {
    node *m_next;

    node_base() : m_next(nullptr) { }
    node_base(const node_base&) = delete;
    node_base operator=(const node_base&) = delete;
  };

  node_base       m_head;
  node_base      *m_tail_p;
  size_t          m_size;
  allocator_type  m_allocator;
};

template <class Tp>
inline void swap(slist<Tp>& a, slist<Tp>& b) { a->swap(b); }

///////////// Implementation ///////////////////

template <typename Tp>
struct slist<Tp>::node : slist<Tp>::node_base {
  union {
    // By putting value into a union, constructor invocation is
    // suppressed, leaving raw bytes that are correctly aligned.
    Tp  m_value;
  };
};

template <typename Tp>
class slist<Tp>::const_iterator {
public:
  using value_type        = Tp;
  using pointer           = Tp const*;
  using reference         = Tp const&;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = forward_iterator_tag;

  reference operator*()  const { return m_prev->m_next->m_value; }
  pointer   operator->() const
    { return std::addressof(m_prev->m_next->m_value); }

  const_iterator& operator++() { m_prev = m_prev->m_next; }
  const_iterator  operator++(int)
    { const_iterator tmp(*this); ++*this; return tmp; }

protected:
  friend class slist<Tp>;

  node_base *m_prev;  // pointer to node before current element

  explicit const_iterator(const node_base *prev)
    : m_prev(const_cast<node_base*>(prev)) { }
};

template <typename Tp>
class slist<Tp>::iterator : public slist<Tp>::const_iterator {
public:
  using pointer           = Tp*;
  using reference         = Tp&;

  reference operator*()  const { return m_prev->m_next->m_value; }
  pointer   operator->() const
    { return std::addressof(m_prev->m_next->m_value); }

private:
  friend class slist<Tp>;
  explicit iterator(node_base *prev) : const_iterator(prev) { }
};

template <typename Tp>
slist<Tp>::slist(allocator_type a)
  : m_head(nullptr), m_tail_p(&m_head), m_size(0), m_allocator(a) { }

template <typename Tp>
slist<Tp>::slist(const slist& other, allocator_type a)
  : slist(a) {
  for (Tp& v : other)
    push_back(v);
}

template <typename Tp>
slist<Tp>::slist(slist&& other)
  : m_head(other.m_head.m_prev)
  , m_tail_p(other.m_tail_p)
  , m_size(other.m_size)
  , m_allocator(other.m_allocator)
{
  other.m_head.m_prev = nullptr;
  other.m_tail_p      = &other.m_head;
  other.m_size        = 0;
}

template <typename Tp>
slist<Tp>::slist(slist&& other, allocator_type a)
  : slist(a)
{
  if (a == m_allocator)
    operator=(std::move(other));
  else
    operator=(other);
}

template <typename Tp>
slist<Tp>::~slist() {
  while (! empty())
    pop_front();
}

template <typename Tp>
slist& slist<Tp>::operator=(const slist& other) {
  if (&other == this) return *this;
  while (! empty())
    pop_front();
  for (Tp& v : other)
    push_back(other);
  return *this;
}

template <typename Tp>
slist& slist<Tp>::operator=(slist&& other) {
  if (m_allocator == other.m_allocator)
    swap(other);
  else
    operator=(other);  // Copy assign
  return *this;
}

template <typename Tp>
void slist<Tp>::swap(slist& other) {
  assert(m_allocator == other.m_allocator);
  using std::swap;
  node_base *tmp = m_head.m_next;
  m_head.m_next = other.m_head.m_next;
  other.m_head.m_next = tmp;
  swap(m_tail_p, other.m_tail_p);
  swap(m_size, other.m_size);
}

template <typename Tp>
template <typename... Args>
void slist<Tp>::emplace(iterator i, Args&&... args) {
  node* new_node = m_allocator.get_resource()->
    allocate(sizeof(Tp), alignof(Tp));
  m_allocator.construct(new_node, std::forward<Args>(args)...);
  new_node.m_next = i.m_prev->m_next;
  i.m_prev->m_next = new_node;
  ++m_size;
}

template <typename Tp>
void slist<Tp>::erase(iterator i) {
  node* old_node = i.m_prev->m_next;
  i.m_prev->m_next = old_node.m_next;
  --m_size;
  m_allocator.destroy(old_node);
  m_allocator.deallocate(old_node, sizeof(Tp), alignof(Tp));
}

#endif // ! defined(INCLUDED_SLIST_DOT_H)

// For best result when pasting into PowerPoint, set indent to 2
// and right-most column to 64.
//
// Local Variables:
// c-basic-offset: 2
// fill-column: 64
// End:
