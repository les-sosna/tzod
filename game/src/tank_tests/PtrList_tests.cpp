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

TEST(PtrList, InsertAt)
{
	PtrList<Foo> pl0;
    PtrList<Foo>::id_type ida = pl0.insert(&a);
    PtrList<Foo>::id_type idb = pl0.insert(&b);
    
    PtrList<Foo> pl1;
    pl1.insert(&b, idb);
    ASSERT_EQ(1, pl1.size());
    ASSERT_EQ(&b, pl1.at(idb));
    pl1.insert(&a, ida);
    ASSERT_EQ(2, pl1.size());
    ASSERT_EQ(&a, pl1.at(ida));
    ASSERT_EQ(&b, pl1.at(idb));
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

TEST(PtrList, ForEach)
{
    PtrList<Foo> pl;
    auto ida = pl.insert(&a);
    auto idb = pl.insert(&b);
    std::map<PtrList<Foo>::id_type, Foo *> check;
    pl.for_each([&](PtrList<Foo>::id_type id, Foo *o) {
        check[id] = o;
    });
    ASSERT_EQ(2, check.size());
    ASSERT_EQ(&a, check[ida]);
    ASSERT_EQ(&b, check[idb]);
}

TEST(PtrList, ForEachEraseOneByOne)
{
    PtrList<Foo> pl;
    pl.insert(&a);
    pl.insert(&b);
    int count = 0;
    pl.for_each([&](PtrList<Foo>::id_type id, Foo *o) {
        pl.erase(id);
        ++count;
    });
    ASSERT_EQ(0, pl.size());
    ASSERT_EQ(2, count);
}

TEST(PtrList, ForEachEraseAll)
{
    PtrList<Foo> pl;
    auto ida = pl.insert(&a);
    auto idb = pl.insert(&b);
    int count = 0;
    pl.for_each([&](PtrList<Foo>::id_type id, Foo *o) {
        pl.erase(ida);
        pl.erase(idb);
        ++count;
    });
    ASSERT_EQ(0, pl.size());
    ASSERT_EQ(1, count);
}

TEST(PtrList, InsertAtRemovedId)
{
    PtrList<Foo> pl;
    Foo foos[10];
    PtrList<Foo>::id_type ids[10];
    for (int i = 0; i != 4; ++i)
        ids[i] = pl.insert(&foos[i]);
    for (int i = 1; i != 3; ++i)
        pl.erase(ids[i]);
    ASSERT_EQ(2, pl.size());
    for (int i = 1; i != 3; ++i)
        pl.insert(&foos[i], ids[i]);
    ASSERT_EQ(4, pl.size());
    int count = 0;
    pl.for_each([&](PtrList<Foo>::id_type, Foo*) {++count;});
    ASSERT_EQ(4, count);
}

TEST(PtrList, InsertAtAfterAdd)
{
    PtrList<Foo> pl;
    Foo foos[10];
    PtrList<Foo>::id_type ids[10];
    for (int i = 0; i != 10; ++i)
        ids[i] = pl.insert(&foos[i]);
    
    PtrList<Foo> pl2;
    pl2.insert(&a);
    auto idb = pl2.insert(&b);
    pl2.insert(&foos[9], ids[9]);
    
    pl2.erase(idb);
    
    ASSERT_EQ(2, pl2.size());
    int count = 0;
    pl2.for_each([&](PtrList<Foo>::id_type, Foo*) {++count;});
    ASSERT_EQ(2, count);
}

