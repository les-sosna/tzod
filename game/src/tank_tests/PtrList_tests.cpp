#include <core/PtrList.h>
#include <gtest/gtest.h>
#include <map>

class Foo{};

static Foo a, b;

TEST(PtrList, Create)
{
	PtrList<Foo> pl;
    ASSERT_TRUE(pl.empty());
    ASSERT_EQ(0, pl.size());
    ASSERT_EQ(pl.end(), pl.begin());
}

TEST(PtrList, Insert)
{
	PtrList<Foo> pl;
    PtrList<Foo>::id_type ida = pl.insert(&a);
    ASSERT_EQ(1, pl.size());
    PtrList<Foo>::id_type idb = pl.insert(&b);
    ASSERT_EQ(2, pl.size());
    ASSERT_EQ(&a, pl.at(ida));
    ASSERT_EQ(&b, pl.at(idb));
}

TEST(PtrList, Begin)
{
	PtrList<Foo> pl;
    auto id = pl.insert(&a);
    ASSERT_EQ(id, pl.begin());
}

TEST(PtrList, Next)
{
	PtrList<Foo> pl;
    std::map<PtrList<Foo>::id_type, Foo*> check;
    check[pl.insert(&a)] = &a;
    check[pl.insert(&b)] = &b;
    for (PtrList<Foo>::id_type id = pl.begin(); id != pl.end(); id = pl.next(id))
    {
        ASSERT_EQ(1, check.count(id));
        ASSERT_EQ(check[id], pl.at(id));
    }
}

TEST(PtrList, Erase)
{
    PtrList<Foo> pl;
    auto ida = pl.insert(&a);
    auto idb = pl.insert(&b);
    pl.erase(ida);
    ASSERT_EQ(1, pl.size());
    ASSERT_EQ(&b, pl.at(idb));
    ASSERT_EQ(idb, pl.begin());
    pl.erase(idb);
    ASSERT_EQ(0, pl.size());
    ASSERT_EQ(pl.end(), pl.begin());
}

TEST(PtrList, EraseBegin)
{
    PtrList<Foo> pl;
    pl.insert(&a);
    pl.insert(&b);
    PtrList<Foo>::id_type next = pl.next(pl.begin());
    pl.erase(pl.begin());
    ASSERT_EQ(pl.begin(), next);
    next = pl.next(pl.begin());
    pl.erase(pl.begin());
    ASSERT_EQ(pl.end(), next);
}

TEST(PtrList, IdReuseAfterErase)
{
    PtrList<Foo> pl;
    auto id1 = pl.insert(&a);
    pl.erase(id1);
    auto id2 = pl.insert(&b);
    ASSERT_EQ(id1, id2);
    ASSERT_EQ(&b, pl.at(id2));
}


