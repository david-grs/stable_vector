#pragma once

#include <vector>
#include <array>
#include <memory>
#include <iterator>
#include <algorithm>

#include <boost/operators.hpp>
#include <boost/container/static_vector.hpp>

template <typename _T, std::size_t _ElementsPerChunk = 512>
struct stable_vector
{
	typedef _T          value_type;
	typedef _T&         reference;
	typedef const _T&   const_reference;
	typedef std::size_t size_type;
	typedef size_type   difference_type;

private:
	typedef boost::container::static_vector<_T, _ElementsPerChunk> chunk_type;
	typedef std::vector<std::unique_ptr<chunk_type>> storage_type;

	typedef stable_vector<_T, _ElementsPerChunk> container;

	template <typename _Iter>
	struct iterator_base
	{
		iterator_base(container* c = nullptr, size_type i = 0) :
			m_container(c),
			m_index(i) {}

		reference operator*() { return (*m_container)[m_index]; }
		iterator_base operator+(size_type i) { return {m_container, m_index + i}; }

		iterator_base& operator+=(size_type i) { m_index += i; return *this; }
		iterator_base& operator++() { ++m_index; return *this; }

		iterator_base& operator-=(size_type i) { m_index -= i; return *this; }
		iterator_base& operator--() { --m_index; return *this; }

		size_type operator-(iterator_base it) { return m_index - it.m_index; }

		bool operator<(iterator_base it) const  { return m_index < it.m_index; }
		bool operator==(iterator_base it) const { return m_container == it.m_container && m_index == it.m_index; }

	protected:
		container* m_container = nullptr;
		size_type  m_index;
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
		{}
	};

	iterator begin() { return {this, 0}; }
	iterator end()   { return {this, size()}; }

	const_iterator cbegin() const { return const_cast<container&>(*this).begin(); }
	const_iterator cend()   const { return const_cast<container&>(*this).end(); }

	size_type size()     const
	{
		return std::accumulate(m_chunks.cbegin(),
							   m_chunks.cend(),
							   0,
							   [](size_type s, auto& chunk_ptr) { return s + chunk_ptr->size(); });
	}
	size_type max_size() const { return std::numeric_limits<size_type>::max(); }
	bool      empty()    const { return m_chunks.empty(); }

	bool operator==(const container& c) const { return size() == c.size() && std::equal(cbegin(), cend(), c.cbegin()); }
	bool operator!=(const container& c) const { return !operator==(c); }

	void swap(container& c) { std::swap(m_chunks, c.m_chunks); }

	friend void swap(container& l, container& r)
	{
		l.swap(r);
	}

	reference front() { return m_chunks.front()->front(); }
	const_reference front() const { return front(); }

	reference back() { return m_chunks.back()->back(); }
	const_reference back() const { return back(); }

private:
	chunk_type& current_chunk()
	{
		if (m_chunks.empty() || m_chunks.back()->size() == _ElementsPerChunk)
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

	template<typename... _Args>
	void emplace_back(_Args&&... args)
	{
		chunk_type& chunk = current_chunk();
		chunk.emplace_back(std::forward<_Args>(args)...);
	}

	reference operator[](size_type i)
	{
		size_type chunk_idx = i / _ElementsPerChunk;
		chunk_type& chunk = *m_chunks[chunk_idx];
		return chunk[i - chunk_idx * _ElementsPerChunk];
	}

	const_reference operator[](size_type i) const { return operator[](i); }

	reference at(size_type i)
	{
		if (i >= size())
			throw std::out_of_range("stable_vector::at");

		return operator[](i);
	}

	const_reference at(size_type i) const { return at(i); }

private:
	storage_type m_chunks;
};

