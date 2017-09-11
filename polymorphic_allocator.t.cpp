/* polymorphic_allocator.t.cpp                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <polymorphic_allocator.h>

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

// Allow compilation of individual test-cases (for test drivers that take a
// very long time to compile).  Specify '-DSINGLE_TEST=<testcase>' to compile
// only the '<testcase>' test case.
#define TEST_IS_ENABLED(num) (! defined(SINGLE_TEST) || SINGLE_TEST == (num))


//=============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
//-----------------------------------------------------------------------------
#define P(X) std::cout << #X " = " << (X) << std::endl; // Print identifier and value.
#define Q(X) std::cout << "<| " #X " |>" << std::endl;  // Quote identifier literally.
#define P_(X) std::cout << #X " = " << (X) << ", " << flush; // P(X) without '\n'
#define L_ __LINE__                           // current Line number
#define T_ std::cout << "\t" << flush;             // Print a tab (w/o newline)

//=============================================================================
//                  GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
//-----------------------------------------------------------------------------

enum { VERBOSE_ARG_NUM = 2, VERY_VERBOSE_ARG_NUM, VERY_VERY_VERBOSE_ARG_NUM };

//=============================================================================
//                  GLOBAL HELPER FUNCTIONS FOR TESTING
//-----------------------------------------------------------------------------
static inline int min(int a, int b) { return a < b ? a : b; }
    // Return the minimum of the specified 'a' and 'b' arguments.

//=============================================================================
//                  CLASSES FOR TESTING USAGE EXAMPLES
//-----------------------------------------------------------------------------

class AllocCounters
{
    int m_num_allocs;
    int m_num_deallocs;
    int m_bytes_allocated;
    int m_bytes_deallocated;

  public:
    AllocCounters()
	: m_num_allocs(0)
	, m_num_deallocs(0)
	, m_bytes_allocated(0)
	, m_bytes_deallocated(0)
	{ }

    void allocate(std::size_t nbytes) {
        ++m_num_allocs;
        m_bytes_allocated += nbytes;
    }

    void deallocate(std::size_t nbytes) {
        ++m_num_deallocs;
        m_bytes_deallocated += nbytes;
    }

    int blocks_outstanding() const { return m_num_allocs - m_num_deallocs; }
    int bytes_outstanding() const
        { return m_bytes_allocated - m_bytes_deallocated; }

    void dump(std::ostream& os, const char* msg) {
	os << msg << ":\n";
        os << "  num allocs         = " << m_num_allocs << '\n';
        os << "  num deallocs       = " << m_num_deallocs << '\n';
        os << "  outstanding allocs = " << blocks_outstanding() << '\n';
        os << "  bytes allocated    = " << m_bytes_allocated << '\n';
        os << "  bytes deallocated  = " << m_bytes_deallocated << '\n';
        os << "  outstanding bytes  = " << bytes_outstanding() << '\n';
        os << std::endl;
    }

    void clear() {
	m_num_allocs = 0;
	m_num_deallocs = 0;
	m_bytes_allocated = 0;
	m_bytes_deallocated = 0;
    }
};

union CountedAllocHeader {
    void*       m_align;
    std::size_t m_size;
};

void *countedAllocate(std::size_t nbytes, AllocCounters *counters)
{
    static const std::size_t headerSize = sizeof(CountedAllocHeader);
    std::size_t blocksize = (nbytes + 2 * headerSize - 1) & ~(headerSize - 1);
    CountedAllocHeader* ret =
        static_cast<CountedAllocHeader*>(std::malloc(blocksize));
    ret->m_size = nbytes;
    ++ret;
    if (counters)
        counters->allocate(nbytes);
    return ret;
}

void countedDeallocate(void *p, std::size_t nbytes, AllocCounters *counters)
{
    CountedAllocHeader* h = static_cast<CountedAllocHeader*>(p) - 1;
    ASSERT(nbytes == h->m_size);
    h->m_size = 0xdeadbeaf;
    std::free(h);
    if (counters)
        counters->deallocate(nbytes);
}

void countedDeallocate(void *p, AllocCounters *counters)
{
    CountedAllocHeader* h = static_cast<CountedAllocHeader*>(p) - 1;
    std::size_t nbytes = h->m_size;
    h->m_size = 0xdeadbeaf;
    std::free(h);
    if (counters)
        counters->deallocate(nbytes);
}

AllocCounters newDeleteCounters;

// Replace global new and delete
void* operator new(std::size_t nbytes)
{
    return countedAllocate(nbytes, &newDeleteCounters);
}

void operator delete(void *p)
{
    countedDeallocate(p, &newDeleteCounters);
}

class TestResource : public cpp17::pmr::memory_resource
{
    AllocCounters m_counters;

    // Not copyable
    TestResource(const TestResource&);
    TestResource operator=(const TestResource&);

public:
    TestResource() { }

    virtual ~TestResource();
    virtual void* allocate(size_t bytes, size_t alignment = 0);
    virtual void  deallocate(void *p, size_t bytes, size_t alignment = 0);
    virtual bool is_equal(const memory_resource& other) const;

    AllocCounters      & counters()       { return m_counters; }
    AllocCounters const& counters() const { return m_counters; }

    void clear() { m_counters.clear(); }
};

TestResource::~TestResource()
{
    ASSERT(0 == m_counters.blocks_outstanding());
}

void* TestResource::allocate(size_t bytes, size_t alignment)
{
    return countedAllocate(bytes, &m_counters);
}

void  TestResource::deallocate(void *p, size_t bytes, size_t alignment)
{
    countedDeallocate(p, bytes, &m_counters);
}

bool TestResource::is_equal(const memory_resource& other) const
{
    // Two TestResource objects are equal only if they are the same object
    return this == &other;
}

TestResource dfltTestRsrc;
AllocCounters& dfltTestCounters = dfltTestRsrc.counters();

AllocCounters dfltSimpleCounters;

template <typename Tp>
class SimpleAllocator
{
    AllocCounters *m_counters;

  public:
    typedef Tp              value_type;

    SimpleAllocator(AllocCounters* c = &dfltSimpleCounters) : m_counters(c) { }

    // Required constructor
    template <typename T>
    SimpleAllocator(const SimpleAllocator<T>& other)
        : m_counters(other.counters()) { }

    Tp* allocate(std::size_t n)
        { return static_cast<Tp*>(countedAllocate(n*sizeof(Tp), m_counters)); }

    void deallocate(Tp* p, std::size_t n)
        { countedDeallocate(p, n*sizeof(Tp), m_counters); }

    AllocCounters* counters() const { return m_counters; }
};

template <typename Tp1, typename Tp2>
bool operator==(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return a.counters() == b.counters();
}

template <typename Tp1, typename Tp2>
bool operator!=(const SimpleAllocator<Tp1>& a, const SimpleAllocator<Tp2>& b)
{
    return ! (a == b);
}

int func(const char* s)
{
    return std::atoi(s);
}

struct UniqDummyType { void zzzzz(UniqDummyType, bool) { } };
typedef void (UniqDummyType::*UniqPointerType)(UniqDummyType);

typedef void (UniqDummyType::*ConvertibleToBoolType)(UniqDummyType, bool);
const ConvertibleToBoolType ConvertibleToTrue = &UniqDummyType::zzzzz;

template <typename _Tp> struct unvoid { typedef _Tp type; };
template <> struct unvoid<void> { struct type { }; };
template <> struct unvoid<const void> { struct type { }; };

template <typename Alloc>
class SimpleString
{
    typedef cpp17::allocator_traits<Alloc> AllocTraits;

    Alloc alloc_;
    typename AllocTraits::pointer data_;

public:
    typedef Alloc allocator_type;

    SimpleString(const Alloc& a = Alloc()) : alloc_(a), data_(nullptr) { }
    SimpleString(const char* s, const Alloc& a = Alloc())
        : alloc_(a), data_(nullptr) { assign(s); }

    SimpleString(const SimpleString& other)
        : alloc_(AllocTraits::select_on_container_copy_construction(
                     other.alloc_))
        , data_(nullptr) { assign(other.data_); }

    SimpleString(SimpleString&& other)
        : alloc_(std::move(other.alloc_))
        , data_(other.data_) { other.data_ = nullptr; }

    SimpleString(const SimpleString& other, const Alloc& a)
        : alloc_(a), data_(nullptr) { assign(other.data_); }

    SimpleString(SimpleString&& other, const Alloc& a)
        : alloc_(a), data_(nullptr)
        { *this = std::move(other); }

    SimpleString& operator=(const SimpleString& other) {
        if (this == &other)
            return *this;
        clear();
        if (AllocTraits::propagate_on_container_copy_assignment::value)
            alloc_ = other.alloc_;
        assign(other.data_);
        return *this;
    }

    SimpleString& operator=(SimpleString&& other) {
        if (this == &other)
            return *this;
        else if (alloc_ == other.alloc_)
            std::swap(data_, other.data_);
        else if (AllocTraits::propagate_on_container_move_assignment::value)
        {
            clear();
            alloc_ = std::move(other.alloc_);
            data_ = other.data_;
            other.data_ = nullptr;
        }
        else
            assign(other.c_str());
        return *this;
    }

    ~SimpleString() { clear(); }

    void clear() {
        if (data_)
            AllocTraits::deallocate(alloc_, data_, std::strlen(&*data_) + 1);
    }

    void assign(const char* s) {
        clear();
        data_ = AllocTraits::allocate(alloc_, std::strlen(s) + 1);
        std::strcpy(&*data_, s);
    }

    const char* c_str() const { return &(*data_); }

    Alloc get_allocator() const { return alloc_; }
};

template <typename Alloc>
inline
bool operator==(const SimpleString<Alloc>& a, const SimpleString<Alloc>& b)
{
    return 0 == std::strcmp(a.c_str(), b.c_str());
}

template <typename Alloc>
inline
bool operator!=(const SimpleString<Alloc>& a, const SimpleString<Alloc>& b)
{
    return ! (a == b);
}

template <typename Alloc>
inline
bool operator==(const SimpleString<Alloc>& a, const char *b)
{
    return 0 == std::strcmp(a.c_str(), b);
}

template <typename Alloc>
inline
bool operator!=(const SimpleString<Alloc>& a, const char *b)
{
    return ! (a == b);
}

template <typename Alloc>
inline
bool operator==(const char *a, const SimpleString<Alloc>& b)
{
    return 0 == std::strcmp(a, b.c_str());
}

template <typename Alloc>
inline
bool operator!=(const char *a, const SimpleString<Alloc>& b)
{
    return ! (a == b);
}

template <typename Tp, typename Alloc = std::allocator<Tp> >
class SimpleVector
{
    // A simple vector-like class with a fixed capacity determined at
    // constuction.

    static_assert(std::is_same<typename Alloc::value_type, Tp>::value,
                  "Allocator's value_type does not match container's");

    typedef cpp17::allocator_traits<Alloc> AllocTraits;

    std::size_t m_capacity;
    std::size_t m_size;
    Alloc       m_alloc;
    Tp         *m_data;

    enum { default_capacity = 5 };

    // No need to make this assignable, so skip implementation
    SimpleVector& operator=(const SimpleVector& other) { return *this; }

public:
    typedef Tp        value_type;
    typedef Alloc     allocator_type;
    typedef Tp       *iterator;
    typedef const Tp *const_iterator;

    SimpleVector(const allocator_type& a = allocator_type());
    SimpleVector(std::size_t capacity,
                 const allocator_type& a = allocator_type());
    SimpleVector(std::size_t size, const value_type& v,
                 const allocator_type& a = allocator_type());
    SimpleVector(const SimpleVector& other);
    SimpleVector(const SimpleVector& other, const allocator_type& a);
    ~SimpleVector();

    void push_back(const value_type& x);
    template <typename... Args>
      void emplace_back(Args&&... args);
    void pop_back();

    Tp      & front()       { return m_data[0]; }
    Tp const& front() const { return m_data[0]; }
    Tp      & back()       { return m_data[m_size - 1]; }
    Tp const& back() const { return m_data[m_size - 1]; }
    Tp      & operator[](std::size_t i)       { return m_data[i]; }
    Tp const& operator[](std::size_t i) const { return m_data[i]; }
    iterator       begin()       { return m_data; }
    const_iterator begin() const { return m_data; }
    iterator       end()       { return m_data + m_size; }
    const_iterator end() const { return m_data + m_size; }

    std::size_t capacity() const { return m_capacity; }
    std::size_t size() const { return m_size; }
    allocator_type get_allocator() const { return m_alloc; }
};

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::SimpleVector(const allocator_type& a)
    : m_capacity(default_capacity)
    , m_size(0)
    , m_alloc(a)
    , m_data(nullptr)
{
    m_data = AllocTraits::allocate(m_alloc, m_capacity);
}

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::SimpleVector(std::size_t capacity,
                                      const allocator_type& a)
    : m_capacity(capacity)
    , m_size(0)
    , m_alloc(a)
    , m_data(nullptr)
{
    m_data = AllocTraits::allocate(m_alloc, m_capacity);
}

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::SimpleVector(std::size_t size, const value_type& v,
                                      const allocator_type& a)
    : m_capacity(size + default_capacity)
    , m_size(0)
    , m_alloc(a)
    , m_data(nullptr)
{
    m_data = AllocTraits::allocate(m_alloc, m_capacity);
    for (std::size_t i = 0; i < size; ++i)
        push_back(v);
}

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::SimpleVector(const SimpleVector& other)
    : m_capacity(other.m_capacity)
    , m_size(0)
    , m_alloc(AllocTraits::select_on_container_copy_construction(other.m_alloc))
    , m_data(nullptr)
{
    m_data = AllocTraits::allocate(m_alloc, m_capacity);
    for (std::size_t i = 0; i < other.m_size; ++i)
        push_back(other[i]);
}

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::SimpleVector(const SimpleVector&   other,
                                      const allocator_type& a)
    : m_capacity(other.m_capacity)
    , m_size(0)
    , m_alloc(a)
    , m_data(nullptr)
{
    m_data = AllocTraits::allocate(m_alloc, m_capacity);
    for (std::size_t i = 0; i < other.m_size; ++i)
        push_back(other[i]);
}

template <typename Tp, typename Alloc>
SimpleVector<Tp, Alloc>::~SimpleVector()
{
    for (std::size_t i = 0; i < m_size; ++i)
        AllocTraits::destroy(m_alloc, cpp17::addressof(m_data[i]));
    AllocTraits::deallocate(m_alloc, m_data, m_capacity);
}

template <typename Tp, typename Alloc>
void SimpleVector<Tp, Alloc>::push_back(const value_type& x)
{
    ASSERT(m_size < m_capacity);
    AllocTraits::construct(m_alloc, cpp17::addressof(m_data[m_size++]), x);
}

template <typename Tp, typename Alloc>
template <typename... Args>
void SimpleVector<Tp, Alloc>::emplace_back(Args&&... args)
{
    ASSERT(m_size < m_capacity);
    AllocTraits::construct(m_alloc, cpp17::addressof(m_data[m_size++]),
                           std::forward<Args>(args)...);
}

template <typename Tp, typename Alloc>
void SimpleVector<Tp, Alloc>::pop_back()
{
    ASSERT(m_size > 0);
    AllocTraits::destroy(m_alloc, cpp17::addressof(m_data[--m_size]));
}

// Force instantiation of whole classes
template class cpp17::pmr::polymorphic_allocator<double>;
template class SimpleVector<double,
                            cpp17::pmr::polymorphic_allocator<double> >;
template class SimpleAllocator<double>;
template class SimpleVector<double, SimpleAllocator<double> >;

//=============================================================================
//                              MAIN PROGRAM
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    using namespace cpp17::pmr;

    int test = argc > 1 ? atoi(argv[1]) : 0;
//     int verbose = argc > 2;
//     int veryVerbose = argc > 3;
//     int veryVeryVerbose = argc > 4;

    std::cout << "TEST " << __FILE__;
    if (test != 0)
        std::cout << " CASE " << test << std::endl;
    else
        std::cout << " all cases" << std::endl;

#define PMA cpp17::pmr::polymorphic_allocator

    switch (test) { case 0: // Do all cases for test-case 0
      case 1:
      {
        // --------------------------------------------------------------------
        // BREATHING TEST
        // --------------------------------------------------------------------

        std::cout << "\nBREATHING TEST"
                  << "\n==============" << std::endl;

        {
            // Test new_delete_resource
            int expBlocks = newDeleteCounters.blocks_outstanding();
            int expBytes = newDeleteCounters.bytes_outstanding();

            memory_resource *r = new_delete_resource_singleton();
            ASSERT(cpp17::pmr::get_default_resource() == r);

            void *p = r->allocate(5);
            ++expBlocks;
            expBytes += 5;

            ASSERT(p);
            ASSERT(newDeleteCounters.blocks_outstanding() == expBlocks);
            ASSERT(newDeleteCounters.bytes_outstanding() == expBytes);
        }

        cpp17::pmr::set_default_resource(&dfltTestRsrc);
        ASSERT(cpp17::pmr::get_default_resource() == &dfltTestRsrc);

        // Test polymorphic allocator constructors
        {
            // Test construction with resource
            TestResource ar;
            const PMA<double> a1(&ar);
            ASSERT(a1.resource() == &ar);

            // Test conversion constructor
            PMA<char> a2(a1);
            ASSERT(a2.resource() == &ar);

            // Test default construction
            PMA<char> a3;
            ASSERT(a3.resource() == &dfltTestRsrc);

            // Test construction with null pointer
            PMA<char> a4(nullptr);
            ASSERT(a4.resource() == &dfltTestRsrc);

            // Test copy constructoin
            PMA<char> a5(a2);
            ASSERT(a5.resource() == &ar);
        }

        TestResource x, y, z;
        AllocCounters &xc = x.counters(), &yc = y.counters();

        newDeleteCounters.clear();

        // Simple use of vector with polymorphic allocator
        {
            typedef PMA<int> Alloc;

            SimpleVector<int, Alloc> vx(&x);
            ASSERT(1 == xc.blocks_outstanding());

            vx.push_back(3);
            ASSERT(3 == vx.front());
            ASSERT(1 == vx.size());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());
            ASSERT(0 == newDeleteCounters.blocks_outstanding());
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == dfltTestCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Outer allocator is polymorphic, inner is not.
        {
            typedef SimpleString<SimpleAllocator<char> > String;
            typedef PMA<String> Alloc;

            SimpleVector<String, Alloc> vx(&x);
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(0 == dfltSimpleCounters.blocks_outstanding());

            vx.push_back("hello");
            ASSERT(1 == vx.size());
            ASSERT("hello" == vx.back());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(1 == dfltSimpleCounters.blocks_outstanding());
            vx.push_back("goodbye");
            ASSERT(2 == vx.size());
            ASSERT("hello" == vx.front());
            ASSERT("goodbye" == vx.back());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(2 == dfltSimpleCounters.blocks_outstanding());
            ASSERT(SimpleAllocator<char>() == vx.front().get_allocator());

            ASSERT(0 == newDeleteCounters.blocks_outstanding());
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == dfltSimpleCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Inner allocator is polymorphic, outer is not.
        {
            typedef SimpleString<PMA<char> > String;
            typedef SimpleAllocator<String> Alloc;

            SimpleVector<String, Alloc> vx(&xc);
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());
            ASSERT(&xc == vx.get_allocator().counters())

            vx.push_back("hello");
            ASSERT(1 == vx.size());
            ASSERT("hello" == vx.back());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(1 == dfltTestCounters.blocks_outstanding());
            vx.push_back("goodbye");
            ASSERT(2 == vx.size());
            ASSERT("hello" == vx.front());
            ASSERT("goodbye" == vx.back());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(2 == dfltTestCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc == vx.front().get_allocator().resource());

            ASSERT(0 == newDeleteCounters.blocks_outstanding());
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == dfltTestCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Both outer and inner allocators are polymorphic.
        {
            typedef SimpleString<PMA<char> > String;
            typedef PMA<String> Alloc;

            SimpleVector<String, Alloc> vx(&x);
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());

            vx.push_back("hello");
            ASSERT(1 == vx.size());
            ASSERT("hello" == vx.back());
            ASSERT(2 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());
            vx.push_back("goodbye");
            ASSERT(2 == vx.size());
            ASSERT("hello" == vx.front());
            ASSERT("goodbye" == vx.back());
            ASSERT(3 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());
            ASSERT(&x == vx.front().get_allocator().resource());

            ASSERT(0 == newDeleteCounters.blocks_outstanding());
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == dfltTestCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Test container copy construction
        {
            typedef SimpleString<PMA<char> > String;
            typedef PMA<String> Alloc;

            SimpleVector<String, Alloc> vx(&x);
            ASSERT(1 == xc.blocks_outstanding());

            vx.push_back("hello");
            vx.push_back("goodbye");
            ASSERT(2 == vx.size());
            ASSERT("hello" == vx.front());
            ASSERT("goodbye" == vx.back());
            ASSERT(3 == xc.blocks_outstanding());
            ASSERT(0 == dfltTestCounters.blocks_outstanding());
            ASSERT(&x == vx.front().get_allocator().resource());

            SimpleVector<String, Alloc> vg(vx);
            ASSERT(2 == vg.size());
            ASSERT("hello" == vg.front());
            ASSERT("goodbye" == vg.back());
            ASSERT(3 == xc.blocks_outstanding());
            ASSERT(3 == dfltTestCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc == vg.front().get_allocator().resource());

            SimpleVector<String, Alloc> vy(vx, &y);
            ASSERT(2 == vy.size());
            ASSERT("hello" == vy.front());
            ASSERT("goodbye" == vy.back());
            ASSERT(3 == xc.blocks_outstanding());
            ASSERT(3 == yc.blocks_outstanding());
            ASSERT(3 == dfltTestCounters.blocks_outstanding());
            ASSERT(&y == vy.front().get_allocator().resource());

            ASSERT(0 == newDeleteCounters.blocks_outstanding());
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == yc.blocks_outstanding());
        ASSERT(0 == dfltTestCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Test resource_adaptor
        {
            typedef SimpleString<PMA<char> > String;
            typedef SimpleVector<String, PMA<String> > strvec;
            typedef SimpleVector<strvec, PMA<strvec> > strvecvec;

            SimpleAllocator<char> sax(&xc);
            SimpleAllocator<char> say(&yc);
            resource_adaptor<SimpleAllocator<char>> crx(sax);
            resource_adaptor<SimpleAllocator<char>> cry(say);

            strvec    a(&crx);
            strvecvec b(&cry);
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(1 == yc.blocks_outstanding());

            ASSERT(0 == a.size());
            a.push_back("hello");
            ASSERT(1 == a.size());
            ASSERT("hello" == a.front());
            a.push_back("goodbye");
            ASSERT(2 == a.size());
            ASSERT("hello" == a.front());
            ASSERT("goodbye" == a.back());
            ASSERT(3 == xc.blocks_outstanding());
            b.push_back(a);
            ASSERT(1 == b.size());
            ASSERT(2 == b.front().size());
            ASSERT("hello" == b.front().front());
            ASSERT("goodbye" == b.front().back());
            ASSERT(3 == xc.blocks_outstanding());
            LOOP_ASSERT(yc.blocks_outstanding(),
                        4 == yc.blocks_outstanding());

            b.emplace_back(3, "repeat");
            ASSERT(2 == b.size());
            ASSERT(3 == b.back().size());
            ASSERT(3 == xc.blocks_outstanding());
            LOOP_ASSERT(yc.blocks_outstanding(),
                        8 == yc.blocks_outstanding());

            static const char* const exp[] = {
                "hello", "goodbye", "repeat", "repeat", "repeat"
            };

            int e = 0;
            for (strvecvec::iterator i = b.begin(); i != b.end(); ++i) {
                ASSERT(i->get_allocator().resource() == &cry);
                for (strvec::iterator j = i->begin(); j != i->end(); ++j) {
                    ASSERT(j->get_allocator().resource() == &cry);
                    ASSERT(*j == exp[e++]);
                }
            }
        }
        LOOP_ASSERT(xc.blocks_outstanding(),
                    0 == xc.blocks_outstanding());
        LOOP_ASSERT(yc.blocks_outstanding(),
                    0 == yc.blocks_outstanding());
        ASSERT(0 == dfltSimpleCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        // Test construct() using pairs
        {
            typedef SimpleString<PMA<char> > String;
            typedef std::pair<String, int> StrInt;
            typedef PMA<StrInt> Alloc;

            SimpleVector<StrInt, SimpleAllocator<StrInt> > vx(&xc);
            SimpleVector<StrInt, Alloc> vy(&y);

            vx.push_back(StrInt("hello", 5));
            ASSERT(1 == vx.size());
            ASSERT(1 == xc.blocks_outstanding());
            ASSERT(1 == dfltTestCounters.blocks_outstanding());
            ASSERT(&dfltTestRsrc==vx.front().first.get_allocator().resource());
            ASSERT("hello" == vx.front().first);
            ASSERT(5 == vx.front().second);
            vy.push_back(StrInt("goodbye", 6));
            ASSERT(1 == vy.size());
            ASSERT(2 == yc.blocks_outstanding());
            ASSERT(1 == dfltTestCounters.blocks_outstanding());
            ASSERT(&y == vy.front().first.get_allocator().resource());
            ASSERT("goodbye" == vy.front().first);
            ASSERT(6 == vy.front().second);
            vy.emplace_back("howdy", 9);
            ASSERT(2 == vy.size());
            ASSERT(3 == yc.blocks_outstanding());
            ASSERT(&y == vy.back().first.get_allocator().resource());
            ASSERT(1 == dfltTestCounters.blocks_outstanding());
            ASSERT("goodbye" == vy[0].first);
            ASSERT(6 == vy[0].second);
            ASSERT("howdy" == vy[1].first);
            ASSERT(9 == vy[1].second);
        }
        ASSERT(0 == xc.blocks_outstanding());
        ASSERT(0 == yc.blocks_outstanding());
        ASSERT(0 == dfltTestCounters.blocks_outstanding());
        ASSERT(0 == dfltSimpleCounters.blocks_outstanding());
        ASSERT(0 == newDeleteCounters.blocks_outstanding());

        cpp17::pmr::set_default_resource(nullptr);
        ASSERT(cpp17::pmr::new_delete_resource_singleton() ==
               cpp17::pmr::get_default_resource());

      } if (test != 0) break;

      break;

      default: {
        std::cerr << "WARNING: CASE `" << test << "' NOT FOUND." << std::endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        std::cerr << "Error, non-zero test status = " << testStatus << "."
                  << std::endl;
    }

    return testStatus;
}
