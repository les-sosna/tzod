#include <core/PtrList.h>
#include <gtest/gtest.h>

class Foo{};

TEST(PtrList, Create)
{
	PtrList<Foo> pl;
}

TEST(PtrList, PushNull)
{
	PtrList<Foo> pl;
    pl.push_back(nullptr);
}

