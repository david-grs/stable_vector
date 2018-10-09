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

namespace internal { namespace stable_vector {

template <class StableVectorT>
class iterator_base
{
public:
	using iterator_category = std::random_access_iterator_tag;

	using size_type = typename StableVectorT::size_type;
	using difference_type = typename StableVectorT::difference_type;

	explicit iterator_base(StableVectorT* container = nullptr, size_type index = 0) :
		m_container(container),
		m_index(index)
	{}

	iterator_base& operator+=(size_type i) { m_index += i; return *this; }
	iterator_base& operator-=(size_type i) { m_index -= i; return *this; }
	iterator_base& operator++()            { ++m_index; return *this; }
	iterator_base& operator--()            { --m_index; return *this; }

	difference_type operator-(const iterator_base& it) { assert(m_container == it.m_container); return m_index - it.m_index; }

	bool operator< (const iterator_base& it) const { assert(m_container == it.m_container); return m_index < it.m_index; }
	bool operator==(const iterator_base& it) const { return m_container == it.m_container && m_index == it.m_index; }

 protected:
	StableVectorT* m_container;
	size_type m_index;
};

template <class StableVectorT>
class const_iterator;

template <class StableVectorT>
class iterator :
	public iterator_base<StableVectorT>,
	public boost::random_access_iterator_helper<iterator<StableVectorT>, typename StableVectorT::value_type>
{
public:
	using value_type = typename StableVectorT::value_type;
	using reference = typename StableVectorT::reference;
	using const_reference = typename StableVectorT::const_reference;

	using iterator_base<StableVectorT>::iterator_base;
	friend class const_iterator<const StableVectorT>;

	reference operator*() { return (*this->m_container)[this->m_index]; }
};

template <class StableVectorT>
class const_iterator :
	public iterator_base<const StableVectorT>,
	public boost::random_access_iterator_helper<const_iterator<const StableVectorT>, const typename StableVectorT::value_type>
{
public:
	using value_type = const typename StableVectorT::value_type;
	using const_reference = typename StableVectorT::const_reference;

	using iterator_base<const StableVectorT>::iterator_base;

	const_iterator(const iterator<StableVectorT>& it) :
		iterator_base<const StableVectorT>(it.m_container, it.m_index)
	{
	}

	const_reference operator*() const { return (*this->m_container)[this->m_index]; }

	bool operator==(const const_iterator& it) const
	{
		return iterator_base<const StableVectorT>::operator==(it);
	}

	friend bool operator==(const iterator<StableVectorT>& l, const const_iterator& r) { return r == l; }
};

}}

template <class T, std::size_t ChunkSize = 1024>
class stable_vector
{
	using __self = stable_vector<T, ChunkSize>;

public:
	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using iterator = internal::stable_vector::iterator<__self>;
	using const_iterator = internal::stable_vector::const_iterator<__self>;

	static constexpr const std::size_t chunk_size = ChunkSize;

private:
	template <std::size_t N>
	struct is_pow2 { static constexpr bool value = (N & (N - 1)) == 0; };

	static_assert(is_pow2<ChunkSize>::value, "ChunkSize needs to be a power of 2");

public:
	stable_vector() = default;
	explicit stable_vector(size_type count, const T& value);
	explicit stable_vector(size_type count);

	template <class InputIt,
			  class = std::enable_if_t<std::is_convertible<typename std::iterator_traits<InputIt>::iterator_category, std::input_iterator_tag>::value>>
	stable_vector(InputIt first, InputIt last);

	stable_vector(std::initializer_list<T>);

	stable_vector(const stable_vector& other);
	stable_vector(stable_vector&& other) noexcept;

	stable_vector& operator=(stable_vector v);

	iterator begin() noexcept { return {this, 0}; }
	const_iterator begin() const noexcept { return {this, 0}; }
	const_iterator cbegin() const noexcept { return begin(); }

	iterator end() noexcept { return {this, size()}; }
	const_iterator end() const noexcept { return {this, size()}; }
	const_iterator cend() const noexcept { return end(); }

	size_type size() const noexcept { return empty() ? 0 : (m_chunks.size() - 1) * ChunkSize + m_chunks.back()->size(); }
	size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
	size_type capacity() const noexcept { return m_chunks.size() * ChunkSize; }

	bool empty() const noexcept { return m_chunks.size() == 0; }

	void reserve(size_type new_capacity);
	void shrink_to_fit() noexcept {}

	bool operator==(const __self& c) const { return size() == c.size() && std::equal(cbegin(), cend(), c.cbegin()); }
	bool operator!=(const __self& c) const { return !operator==(c); }

	void swap(__self& v) { std::swap(m_chunks, v.m_chunks); }

	friend void swap(__self& l, __self& r) { l.swap(r); }

	reference front()             { return m_chunks.front()->front(); }
	const_reference front() const { return front(); }

	reference back()             { return m_chunks.back()->back(); }
	const_reference back() const { return back(); }

	void push_back(const T& t);
	void push_back(T&& t);

	template <class... Args>
	void emplace_back(Args&&... args);

	reference operator[](size_type i);

	const_reference operator[](size_type i) const;

	reference at(size_type i);

	const_reference at(size_type i) const;

private:
	using chunk_type = boost::container::static_vector<T, ChunkSize>;
	using storage_type = std::vector<std::unique_ptr<chunk_type>>;

	void add_chunk();
	chunk_type& last_chunk();

	storage_type m_chunks;
};







template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>::stable_vector(size_type count, const T& value)
{
	for (size_type i = 0; i < count; ++i)
	{
		push_back(value);
	}
}

template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>::stable_vector(size_type count)
{
	for (size_type i = 0; i < count; ++i)
	{
		emplace_back();
	}
}

template <class T, std::size_t ChunkSize>
template <class InputIt, class>
stable_vector<T, ChunkSize>::stable_vector(InputIt first, InputIt last)
{
	for (; first != last; ++first)
	{
		push_back(*first);
	}
}

template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>::stable_vector(const stable_vector& other)
{
	for (const auto& chunk : other.m_chunks)
	{
		m_chunks.emplace_back(std::make_unique<chunk_type>(*chunk));
	}
}

template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>::stable_vector(stable_vector&& other) noexcept :
	m_chunks(std::move(other.m_chunks))
{
}

template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>::stable_vector(std::initializer_list<T> ilist)
{
	for (const auto& t : ilist)
	{
		push_back(t);
	}
}

template <class T, std::size_t ChunkSize>
stable_vector<T, ChunkSize>& stable_vector<T, ChunkSize>::operator=(stable_vector v)
{
	swap(v);
	return *this;
}

template <class T, std::size_t ChunkSize>
void stable_vector<T, ChunkSize>::add_chunk()
{
	m_chunks.emplace_back(std::make_unique<chunk_type>());
}

template <class T, std::size_t ChunkSize>
typename stable_vector<T, ChunkSize>::chunk_type& stable_vector<T, ChunkSize>::last_chunk()
{
	if (likely_false(m_chunks.empty() || m_chunks.back()->size() == ChunkSize))
	{
		add_chunk();
	}

	return *m_chunks.back();
}

template <class T, std::size_t ChunkSize>
void stable_vector<T, ChunkSize>::reserve(size_type new_capacity)
{
	const std::size_t initial_capacity = capacity();
	for (difference_type i = new_capacity - initial_capacity; i > 0; i -= ChunkSize)
	{
		add_chunk();
	}
}

template <class T, std::size_t ChunkSize>
void stable_vector<T, ChunkSize>::push_back(const T& t)
{
	last_chunk().push_back(t);
}

template <class T, std::size_t ChunkSize>
void stable_vector<T, ChunkSize>::push_back(T&& t)
{
	last_chunk().push_back(std::move(t));
}

template <class T, std::size_t ChunkSize>
template <class... Args>
void stable_vector<T, ChunkSize>::emplace_back(Args&&... args)
{
	last_chunk().emplace_back(std::forward<Args>(args)...);
}

template <class T, std::size_t ChunkSize>
typename stable_vector<T, ChunkSize>::reference
stable_vector<T, ChunkSize>::operator[](size_type i)
{
	return (*m_chunks[i / ChunkSize])[i % ChunkSize];
}

template <class T, std::size_t ChunkSize>
typename stable_vector<T, ChunkSize>::const_reference
stable_vector<T, ChunkSize>::operator[](size_type i) const
{
	return const_cast<__self&>(*this)[i];
}

template <class T, std::size_t ChunkSize>
typename stable_vector<T, ChunkSize>::reference
stable_vector<T, ChunkSize>::at(size_type i)
{
	if (likely_false(i >= size()))
	{
		throw std::out_of_range("stable_vector::at");
	}

	return operator[](i);
}

template <class T, std::size_t ChunkSize>
typename stable_vector<T, ChunkSize>::const_reference
stable_vector<T, ChunkSize>::at(size_type i) const
{
	return const_cast<__self&>(*this).at(i);
}

