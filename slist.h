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
#include <algorithm>
#include <cassert>

namespace pmr = cpp17::pmr;

template <typename Tp> class slist;

namespace slist_details {

template <typename Tp> struct node;

template <typename Tp>
struct node_base {
  node<Tp> *m_next;

  node_base() : m_next(nullptr) { }
  node_base(const node_base&) = delete;
  node_base operator=(const node_base&) = delete;
};

template <typename Tp>
struct node : node_base<Tp> {
  union {
    // By putting value into a union, constructor invocation is
    // suppressed, leaving raw bytes that are correctly aligned.
    Tp  m_value;
  };
};

template <typename Tp>
class const_iterator {
public:
  using value_type        = Tp;
  using pointer           = Tp const*;
  using reference         = Tp const&;
  using difference_type   = std::ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;

  reference operator*()  const { return m_prev->m_next->m_value; }
  pointer   operator->() const
    { return std::addressof(m_prev->m_next->m_value); }

  const_iterator& operator++()
    { m_prev = m_prev->m_next; return *this;}
  const_iterator  operator++(int)
    { const_iterator tmp(*this); ++*this; return tmp; }

  bool operator==(const_iterator other) const
    { return m_prev == other.m_prev; }
  bool operator!=(const_iterator other) const
    { return ! operator==(other); }

protected:
  friend class slist<Tp>;

  node_base<Tp> *m_prev;  // pointer to node before current element

  explicit const_iterator(const node_base<Tp> *prev)
    : m_prev(const_cast<node_base<Tp>*>(prev)) { }
};

template <typename Tp>
class iterator : public const_iterator<Tp> {
  using Base = const_iterator<Tp>;

public:
  using pointer           = Tp*;
  using reference         = Tp&;

  reference operator*()  const
    { return this->m_prev->m_next->m_value; }
  pointer   operator->() const
    { return std::addressof(this->m_prev->m_next->m_value); }

  iterator& operator++() { Base::operator++(); return *this; }
  iterator  operator++(int)
    { iterator tmp(*this); ++*this; return tmp; }

private:
  friend class slist<Tp>;
  explicit iterator(node_base<Tp> *prev) : const_iterator<Tp>(prev) { }
};

} // close namespace slist_details

// Singly-linked list that supports the use of a polymorphic
// allocator.
template <typename Tp>
class slist {
  using byte = cpp17::byte;
public:
  using value_type     = Tp;
  using allocator_type = pmr::polymorphic_allocator<byte>;
  using iterator       = slist_details::iterator<Tp>;
  using const_iterator = slist_details::const_iterator<Tp>;

  slist(allocator_type a = {});
  slist(const slist& other, allocator_type a = {});
  slist(slist&& other);
  slist(slist&& other, allocator_type a);
  ~slist();

  slist& operator=(const slist& other);
  slist& operator=(slist&& other);
  void swap(slist& other);

  size_t size() const { return m_size; }
  bool   empty() const { return 0 == m_size; }

  iterator begin()              { return iterator(&m_head); }
  iterator end()                { return iterator(m_tail_p); }
  const_iterator begin() const  { return const_iterator(&m_head); }
  const_iterator end() const    { return const_iterator(m_tail_p); }
  const_iterator cbegin() const { return const_iterator(&m_head); }
  const_iterator cend() const   { return const_iterator(m_tail_p); }

  Tp      & front()       { return m_head.m_next->m_value; }
  Tp const& front() const { return m_head.m_next->m_value; }

  template <typename... Args>
    iterator emplace(iterator i, Args&&... args);
  template <typename... Args> void emplace_front(Args&&... args)
    { emplace(begin(), std::forward<Args>(args)...); }
  template <typename... Args> void emplace_back(Args&&... args)
    { emplace(end(), std::forward<Args>(args)...); }

  iterator insert(iterator i, const Tp& v) { return emplace(i, v); }
  void push_front(const Tp& v)         { emplace(begin(), v); }
  void push_back(const Tp& v)          { emplace(end(), v); }

  // Note: erasing elements invalidates iterators to the node
  // following the element being erased.
  iterator erase(iterator b, iterator e);
  iterator erase(iterator i) { iterator e = i; return erase(i, ++e); }
  void pop_front() { erase(begin()); }

  allocator_type get_allocator() const { return m_allocator; }

private:
  using node_base = slist_details::node_base<Tp>;
  using node      = slist_details::node<Tp>;

  node_base       m_head;
  node_base      *m_tail_p;
  size_t          m_size;
  allocator_type  m_allocator;
};

template <class Tp>
inline void swap(slist<Tp>& a, slist<Tp>& b) { a.swap(b); }

template <class Tp>
inline bool operator==(const slist<Tp>& a, const slist<Tp>& b) {
  if (a.size() != b.size())
    return false;
  else
    return std::equal(a.begin(), a.end(), b.begin());
}

template <class Tp>
inline bool operator!=(const slist<Tp>& a, const slist<Tp>& b) {
  return ! (a == b);
}

///////////// Implementation ///////////////////

template <typename Tp>
slist<Tp>::slist(allocator_type a)
  : m_head(), m_tail_p(&m_head), m_size(0), m_allocator(a) { }

template <typename Tp>
slist<Tp>::slist(const slist& other, allocator_type a)
  : slist(a) {
  for (const Tp& v : other)
    push_back(v);
}

template <typename Tp>
slist<Tp>::slist(slist&& other)
  : m_head()
  , m_tail_p(other.m_tail_p)
  , m_size(other.m_size)
  , m_allocator(other.m_allocator)
{
  m_head.m_next       = other.m_head.m_next;
  other.m_head.m_next = nullptr;
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
  erase(begin(), end());
}

template <typename Tp>
slist<Tp>& slist<Tp>::operator=(const slist& other) {
  if (&other == this) return *this;
  erase(begin(), end());
  for (const Tp& v : other)
    push_back(v);
  return *this;
}

template <typename Tp>
slist<Tp>& slist<Tp>::operator=(slist&& other) {
  if (m_allocator == other.m_allocator) {
    erase(begin(), end());
    swap(other);
  }
  else
    operator=(other);  // Copy assign
  return *this;
}

template <typename Tp>
void slist<Tp>::swap(slist& other) {
  assert(m_allocator == other.m_allocator);
  using std::swap;
  node *tmp = m_head.m_next;
  m_head.m_next = other.m_head.m_next;
  other.m_head.m_next = tmp;
  swap(m_tail_p, other.m_tail_p);
  swap(m_size, other.m_size);
}

template <typename Tp>
template <typename... Args>
typename slist<Tp>::iterator
slist<Tp>::emplace(iterator i, Args&&... args) {
  node* new_node = static_cast<node*>(
    m_allocator.resource()->allocate(sizeof(node), alignof(node)));
  m_allocator.construct(std::addressof(new_node->m_value),
                        std::forward<Args>(args)...);

  new_node->m_next = i.m_prev->m_next;
  i.m_prev->m_next = new_node;
  if (i.m_prev == m_tail_p)
    m_tail_p = new_node;  // Added at end
  ++m_size;
  return i;
}

template <typename Tp>
typename slist<Tp>::iterator
slist<Tp>::erase(iterator b, iterator e) {
  node *erase_next = b.m_prev->m_next;
  node *erase_past = e.m_prev->m_next; // one past last erasure
  if (nullptr == erase_past)
    m_tail_p = b.m_prev;  // Erasing at tail
  b.m_prev->m_next = erase_past; // splice out sublist
  while (erase_next != erase_past) {
    node* old_node = erase_next;
    erase_next = erase_next->m_next;
    --m_size;
    m_allocator.destroy(std::addressof(old_node->m_value));
    m_allocator.resource()->deallocate(old_node,
                                       sizeof(node), alignof(node));
  }

  return b;
}

#endif // ! defined(INCLUDED_SLIST_DOT_H)

// For best result when pasting into PowerPoint, set indent to 2
// and right-most column to 64.
//
// Local Variables:
// c-basic-offset: 2
// fill-column: 64
// End:
