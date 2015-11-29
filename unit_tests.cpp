#include "stable_vector.h"

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
