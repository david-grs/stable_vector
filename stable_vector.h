#pragma once

#include <type_traits>
#include <vector>
#include <initializer_list>
#include <memory>
#include <iterator>
#include <algorithm>
#include <numeric>

#include <boost/operators.hpp>
#include <boost/container/static_vector.hpp>

#define likely_false(x) __builtin_expect((x), 0)
#define likely_true(x)  __builtin_expect((x), 1)

template <typename T, std::size_t _ChunkSize = 512>
struct stable_vector
{
	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	constexpr size_type chunk_size() const noexcept { return _ChunkSize; }
	constexpr size_type max_size() const noexcept   { return std::numeric_limits<size_type>::max(); }

private:
	using chunk_type = boost::container::static_vector<T, _ChunkSize>;
	using storage_type = std::vector<std::unique_ptr<chunk_type>>;

	using container = stable_vector<T, _ChunkSize>;

	template <typename _Iter, typename _ContainerT>
	struct iterator_base
	{
		iterator_base(_ContainerT* c = nullptr, size_type i = 0) :
			m_container(c),
			m_index(i)
		{}
		virtual ~iterator_base() {}

		iterator_base& operator+=(size_type i) { m_index += i; return *this; }
		iterator_base& operator-=(size_type i) { m_index -= i; return *this; }
		iterator_base& operator++()            { ++m_index; return *this; }
		iterator_base& operator--()            { --m_index; return *this; }

		difference_type operator-(const iterator_base& it) { assert(m_container == it.m_container); return m_index - it.m_index; }

		bool operator< (const iterator_base& it) const { assert(m_container == it.m_container); return m_index < it.m_index; }
		bool operator==(const iterator_base& it) const { return m_container == it.m_container && m_index == it.m_index; }

	 protected:
		_ContainerT* m_container;
		size_type m_index;
	};

public:
	struct const_iterator;

	struct iterator :
		public iterator_base<iterator, container>,
		public boost::random_access_iterator_helper<iterator, value_type>
	{
		using iterator_base<iterator, container>::iterator_base;
		friend struct const_iterator;

		reference operator*() { return (*this->m_container)[this->m_index]; }
	};

	struct const_iterator :
		public iterator_base<const_iterator, const container>,
		public boost::random_access_iterator_helper<const_iterator, const value_type>
	{
		using iterator_base<const_iterator, const container>::iterator_base;

		const_iterator(const iterator& it) :
			iterator_base<const_iterator, const container>(it.m_container, it.m_index)
		{
		}

		const_reference operator*() const { return (*this->m_container)[this->m_index]; }

		bool operator==(const const_iterator& it) const
		{ return iterator_base<const_iterator, const container>::operator==(it); }

		friend bool operator==(const iterator& l, const const_iterator& r) { return r == l; }
	};

	stable_vector() = default;

	explicit stable_vector(size_type count, const T& value)
	{
		for (size_type i = 0; i < count; ++i)
			push_back(value);
	}

	explicit stable_vector(size_type count)
	{
		for (size_type i = 0; i < count; ++i)
			emplace_back();
	}

	template <typename InputIt,
			  typename =
				  typename std::enable_if<
					  std::is_convertible<
						  typename std::iterator_traits<InputIt>::iterator_category, std::input_iterator_tag
			  >::value>::type>
	stable_vector(InputIt first, InputIt last)
	{
		for (; first != last; ++first)
			push_back(*first);
	}

	stable_vector(const stable_vector& other)
	{
		for (const auto& chunk : other.m_chunks)
			m_chunks.emplace_back(std::make_unique<chunk_type>(*chunk));
	}

	stable_vector(stable_vector&& other) :
		m_chunks(std::move(other.m_chunks))
	{
	}

	stable_vector(std::initializer_list<T> ilist)
	{
		for (const auto& t : ilist)
			push_back(t);
	}

	stable_vector& operator=(stable_vector v)
	{
		swap(v);
		return *this;
	}

	iterator begin() { return {this, 0}; }
	iterator end()   { return {this, size()}; }

	const_iterator begin() const { return {this, 0}; }
	const_iterator end()   const { return {this, size()}; }

	const_iterator cbegin() const { return begin(); }
	const_iterator cend()   const { return end(); }

	size_type size() const
	{
		return std::accumulate(m_chunks.cbegin(), m_chunks.cend(), size_type{}, [](size_type s, auto& chunk_ptr)
							   {
								   return s + chunk_ptr->size();
							   });
	}

	size_type capacity() const { return m_chunks.size() * _ChunkSize; }
	bool empty() const { return m_chunks.empty(); }

	bool operator==(const container& c) const
	{
		return size() == c.size() && std::equal(cbegin(), cend(), c.cbegin());
	}

	bool operator!=(const container& c) const { return !operator==(c); }

	void swap(container& v) { std::swap(m_chunks, v.m_chunks); }

	friend void swap(container& l, container& r) { l.swap(r); }

	reference front()             { return m_chunks.front()->front(); }
	const_reference front() const { return front(); }

	reference back()             { return m_chunks.back()->back(); }
	const_reference back() const { return back(); }

private:
	void add_chunk() {  m_chunks.emplace_back(std::make_unique<chunk_type>()); }

	chunk_type& current_chunk()
	{
		if (likely_false(m_chunks.empty() || m_chunks.back()->size() == _ChunkSize))
		   add_chunk();

		return *m_chunks.back();
	}

public:
	void reserve(size_type sz)
	{
		for (std::size_t i = sz - capacity(); i > 0; i -= _ChunkSize)
			add_chunk();
	}

	void push_back(const T& t) { current_chunk().push_back(t); }
	void push_back(T&& t)      { current_chunk().push_back(std::move(t)); }

	template <typename... _Args>
	void emplace_back(_Args&&... args)
	{
		current_chunk().emplace_back(std::forward<_Args>(args)...);
	}

	reference operator[](size_type i)
	{
		size_type chunk_idx = i / _ChunkSize;
		chunk_type& chunk = *m_chunks[chunk_idx];
		return chunk[i - chunk_idx * _ChunkSize];
	}

	const_reference operator[](size_type i) const
	{
		return const_cast<container&>(*this)[i];
	}

	reference at(size_type i)
	{
		if (likely_false(i >= size()))
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
