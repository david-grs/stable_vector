#pragma once

#include <vector>
#include <initializer_list>
#include <memory>
#include <iterator>
#include <algorithm>
#include <numeric>

#include <boost/operators.hpp>
#include <boost/container/static_vector.hpp>

template <typename _T, std::size_t _ChunkSize = 512>
struct stable_vector
{
    using value_type = _T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using size_type = std::size_t;
    using difference_type = size_type;

    constexpr size_type chunk_size() const noexcept { return _ChunkSize; }

private:
    using chunk_type = boost::container::static_vector<_T, _ChunkSize>;
    using storage_type = std::vector<std::unique_ptr<chunk_type>>;

    using container = stable_vector<_T, _ChunkSize>;

    template <typename _Iter>
    struct iterator_base
    {
        iterator_base(container* c = nullptr, size_type i = 0) :
            m_container(c),
            m_index(i)
        {}
        virtual ~iterator_base() {}

        reference operator*() { return (*m_container)[m_index]; }

        iterator_base& operator+=(size_type i) { m_index += i; return *this; }
        iterator_base& operator-=(size_type i) { return operator+=(-i); }
        iterator_base& operator++()            { return operator+=(1); }
        iterator_base& operator--()            { return operator+=(-1); }

        difference_type operator-(const iterator_base& it) { assert(m_container == it.m_container); return m_index - it.m_index; }

        bool operator< (const iterator_base& it) const { assert(m_container == it.m_container); return m_index < it.m_index; }
        bool operator==(const iterator_base& it) const { return m_container == it.m_container && m_index == it.m_index; }

     protected:
        container* m_container;
        size_type m_index;
    };

public:
    struct const_iterator;

    struct iterator :
        public iterator_base<iterator>,
        public boost::random_access_iterator_helper<iterator, value_type>
    {
        using iterator_base<iterator>::iterator_base;
        friend struct const_iterator;
    };

    struct const_iterator :
        public iterator_base<const_iterator>,
        public boost::random_access_iterator_helper<const_iterator, const value_type>
    {
        using iterator_base<const_iterator>::iterator_base;

        const_iterator(const iterator& it) :
            iterator_base<const_iterator>(it.m_container, it.m_index)
        {
        }

        bool operator==(const const_iterator& it) const { return iterator_base<const_iterator>::operator==(it); }

        friend bool operator==(const iterator& l, const const_iterator& r) { return r == l; }
    };

    stable_vector() = default;

    explicit stable_vector(size_type count, const _T& value)
    {
        std::fill_n(begin(), count, value);
    }

    explicit stable_vector(size_type count)
    {
        for (size_type i = 0; i < count; ++i)
            emplace_back();
    }

    stable_vector(const std::initializer_list<_T>& init)
    {
        for (const auto& t : init)
            push_back(t);
    }

    template <typename _InputIt,
              typename =
                  typename std::enable_if<
                      std::is_convertible<
                          typename std::iterator_traits<_InputIt>::iterator_category, std::input_iterator_tag
              >::value>::type>
    stable_vector(_InputIt first, const _InputIt& last)
    {
        for (; first != last; ++first)
            push_back(*first);
    }

    stable_vector(const stable_vector& v)
    {
        for (const std::unique_ptr<chunk_type>& pchunk : v.m_chunks)
            m_chunks.emplace_back(new chunk_type(*pchunk));
    }

    stable_vector(stable_vector&& v) : m_chunks(std::move(v.m_chunks))
    {
    }

    stable_vector& operator=(stable_vector v)
    {
        swap(v);
        return *this;
    }

    iterator begin()
    {
        return {this, 0};
    }
    iterator end()
    {
        return {this, size()};
    }

    const_iterator cbegin() const
    {
        return const_cast<container&>(*this).begin();
    }
    const_iterator cend() const
    {
        return const_cast<container&>(*this).end();
    }

    size_type size() const
    {
        return std::accumulate(m_chunks.cbegin(), m_chunks.cend(), 0, [](size_type s, auto& chunk_ptr)
                               {
                                   return s + chunk_ptr->size();
                               });
    }
    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max();
    }
    bool empty() const
    {
        return m_chunks.empty();
    }

    bool operator==(const container& c) const
    {
        return size() == c.size() && std::equal(cbegin(), cend(), c.cbegin());
    }
    bool operator!=(const container& c) const
    {
        return !operator==(c);
    }

    void swap(container& v)
    {
        std::swap(m_chunks, v.m_chunks);
    }

    friend void swap(container& l, container& r)
    {
        l.swap(r);
    }

    reference front()
    {
        return m_chunks.front()->front();
    }
    const_reference front() const
    {
        return front();
    }

    reference back()
    {
        return m_chunks.back()->back();
    }
    const_reference back() const
    {
        return back();
    }

private:
    chunk_type& current_chunk()
    {
        if (m_chunks.empty() || m_chunks.back()->size() == _ChunkSize)
            m_chunks.emplace_back(new chunk_type());

        return *m_chunks.back();
    }

public:
    void push_back(const _T& t)
    {
        chunk_type& chunk = current_chunk();
        chunk.push_back(t);
    }

    void push_back(_T&& t)
    {
        chunk_type& chunk = current_chunk();
        chunk.push_back(std::move(t));
    }

    template <typename... _Args>
    void emplace_back(_Args&&... args)
    {
        chunk_type& chunk = current_chunk();
        chunk.emplace_back(std::forward<_Args>(args)...);
    }

    reference operator[](size_type i)
    {
        size_type chunk_idx = i / _ChunkSize;
        chunk_type& chunk = *m_chunks[chunk_idx];
        return chunk[i - chunk_idx * _ChunkSize];
    }

    const_reference operator[](size_type i) const
    {
        return operator[](i);
    }

    reference at(size_type i)
    {
        if (i >= size())
            throw std::out_of_range("stable_vector::at");

        return operator[](i);
    }

    const_reference at(size_type i) const
    {
        return at(i);
    }

private:
    storage_type m_chunks;
};
