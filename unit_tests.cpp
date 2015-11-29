#include "stable_vector.h"

#include <list>

#include <boost/noncopyable.hpp>
#include <gtest/gtest.h>

struct A
{
	explicit A(int i) :
		m_i(i) {}
	int m_i;
};

struct B : A, boost::noncopyable { using A::A; };

TEST(stable_vector, init)
{
	stable_vector<int> v;
	ASSERT_TRUE(v.empty());
	ASSERT_EQ(v.size(), 0);
}

TEST(stable_vector, ctor_initializer_list)
{
	stable_vector<int, 10> v = {0,1,2,3,4};
	ASSERT_EQ(v.size(), 5);
	ASSERT_EQ(std::accumulate(v.cbegin(), v.cend(), 0), 0+1+2+3+4);
}

TEST(stable_vector, ctor_element_copies)
{
	stable_vector<int, 10> v(5, 1);
	ASSERT_EQ(v.size(), 5);
	ASSERT_EQ(v[0], 1);
	ASSERT_EQ(std::accumulate(v.cbegin(), v.cend(), 0), 5);
}

TEST(stable_vector, ctor_count)
{
	stable_vector<int, 10> v(5);
	ASSERT_EQ(v.size(), 5);
	ASSERT_EQ(v[0], 0);
	ASSERT_EQ(std::accumulate(v.cbegin(), v.cend(), 0), 0);
}

TEST(stable_vector, ctor_input_iterator)
{
	std::list<int> l = {1,2,3,4,5};
	stable_vector<int, 10> v(l.begin(), l.end());

	ASSERT_EQ(v.size(), l.size());
	ASSERT_EQ(std::accumulate(v.cbegin(), v.cend(), 0),
			  std::accumulate(l.cbegin(), l.cend(), 0));
}

TEST(stable_vector, copy_ctor)
{
	stable_vector<int, 10> v1 = {1,2,3,4,5};
	stable_vector<int, 10> v2(v1);

	ASSERT_TRUE(v1 == v2);

	v2.push_back(6);
	ASSERT_EQ(v1.size(), 5);
	ASSERT_EQ(v2.size(), 6);
}

TEST(stable_vector, copy_assignment)
{
	stable_vector<int, 10> v1 = {1,2,3,4,5};
	stable_vector<int, 10> v2 = {10,11};

	ASSERT_TRUE(v1 != v2);

	v2 = v1;

	ASSERT_TRUE(v1 == v2);

	v2.push_back(6);
	ASSERT_EQ(v1.size(), 5);
	ASSERT_EQ(v2.size(), 6);
}

TEST(stable_vector, move_ctor)
{
	stable_vector<int, 10> vtmp = {1,2,3,4,5};
	stable_vector<int, 10> v2(std::move(vtmp));

	ASSERT_EQ(v2.size(), 5);
	ASSERT_TRUE(vtmp.empty());
}

TEST(stable_vector, move_assignment)
{
	stable_vector<int, 10> v2 = {10, 11};

	v2 = stable_vector<int, 10>({1,2,3,4,5});

	ASSERT_EQ(v2.size(), 5);
}

TEST(stable_vector, push_back)
{
	stable_vector<A> v;

	A a(1);
	v.push_back(a);

	ASSERT_EQ(v.size(), 1);
	ASSERT_EQ(v[0].m_i, a.m_i);

	v.push_back(A(2));

	ASSERT_EQ(v.size(), 2);
	ASSERT_EQ(v[0].m_i, a.m_i);
	ASSERT_EQ(v[1].m_i, 2);
}

TEST(stable_vector, emplace_back)
{
	stable_vector<B, 10> v;

	v.emplace_back(1);

	ASSERT_EQ(v.size(), 1);
	ASSERT_EQ(v[0].m_i, 1);
}

TEST(stable_vector, out_of_range)
{
	stable_vector<A, 10> v;
	ASSERT_THROW(v.at(0), std::out_of_range);
}

TEST(stable_vector, equal)
{
	stable_vector<int, 10> v1;
	v1.push_back(0);
	v1.push_back(1);
	v1.push_back(2);

	stable_vector<int, 10> v2;
	v2.push_back(0);
	v2.push_back(1);
	v2.push_back(2);

	ASSERT_TRUE(v1 == v2);
	ASSERT_FALSE(v1 != v2);
}

TEST(stable_vector, not_equal)
{
	stable_vector<int, 10> v1;
	v1.push_back(0);

	stable_vector<int, 10> v2;

	ASSERT_TRUE(v1 != v2);
	ASSERT_FALSE(v1 == v2);
}

TEST(stable_vector, front)
{
	stable_vector<int, 10> v;
	v.push_back(1);
	ASSERT_EQ(v.front(), 1);

	v.push_back(2);
	ASSERT_EQ(v.front(), 1);
}

TEST(stable_vector, back)
{
	stable_vector<int, 10> v;
	v.push_back(1);
	ASSERT_EQ(v.back(), 1);

	v.push_back(2);
	ASSERT_EQ(v.back(), 2);
}

TEST(stable_vector, begin)
{
	stable_vector<int, 10> v;
	v.push_back(1);
	ASSERT_EQ(*v.begin(), 1);
	ASSERT_EQ(*v.cbegin(), 1);

	v.push_back(2);
	ASSERT_EQ(*v.begin(), 1);
	ASSERT_EQ(*v.cbegin(), 1);
}

TEST(stable_vector, end)
{
	stable_vector<int, 10> v;
	v.push_back(1);
	ASSERT_EQ(*(v.end() - 1), 1);
	ASSERT_EQ(*(v.cend() - 1), 1);

	v.push_back(2);
	ASSERT_EQ(*(v.end() - 1), 2);
	ASSERT_EQ(*(v.cend() - 1), 2);
}

TEST(stable_vector_iterator, empty)
{
	stable_vector<int, 10> v;
	ASSERT_TRUE(v.begin() == v.end());
	ASSERT_TRUE(v.begin() == v.cend());
	ASSERT_TRUE(v.cbegin() == v.end());
	ASSERT_TRUE(v.cbegin() == v.cend());
}

TEST(stable_vector_iterator, for_loop)
{
	stable_vector<int, 10> v = {0,1,2,3,4};
	int i = 0;

	for (auto it = v.cbegin(); it != v.cend(); ++it, ++i)
		ASSERT_EQ(*it, i);

	i = 0;
	for (auto t : v)
		ASSERT_EQ(t, i++);
}

TEST(stable_vector_iterator, arithmetic)
{
	stable_vector<int, 10> v = {0,1,2,3,4};
	auto it = v.cbegin() + 3;
	ASSERT_EQ(*it, 3);

	it = it - 1;
	ASSERT_EQ(*it, 2);

	--it;
	ASSERT_EQ(*it, 1);

	it += 4;
	ASSERT_TRUE(it == v.cend());
	ASSERT_TRUE(it == v.end());

	it -= 5;
	ASSERT_TRUE(it == v.cbegin());
	ASSERT_TRUE(it == v.begin());
}