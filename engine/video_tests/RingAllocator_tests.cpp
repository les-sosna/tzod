#include <video/RingAllocator.h>
#include <gtest/gtest.h>

TEST(RingAllocator, InitWithCapacity)
{
    RingAllocator a(128);
}

TEST(RingAllocator, AllocateSome)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(13, 1), 0);
    EXPECT_EQ(a.Allocate(14, 1), 13);
    EXPECT_EQ(a.Allocate(3, 16), 32);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
}

TEST(RingAllocator, AllocateAndCommitNonOverlapped)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(13, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    EXPECT_EQ(a.Allocate(14, 1), 13);
    EXPECT_EQ(a.CommitFreeOverlapped(1002), 0);
    EXPECT_EQ(a.Allocate(3, 16), 32);
    EXPECT_EQ(a.CommitFreeOverlapped(1003), 0);
}

TEST(RingAllocator, AllocateCommitFullCapacityAtOnceDifferentAlignments)
{
    {
        RingAllocator a(128);
        EXPECT_EQ(a.Allocate(128, 1), 0);
        EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    }
    {
        RingAllocator a(128);
        EXPECT_EQ(a.Allocate(128, 16), 0);
        EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    }
    {
        RingAllocator a(128);
        EXPECT_EQ(a.Allocate(128, 128), 0);
        EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    }
}

TEST(RingAllocator, AllocateFullCapacitySeveralAllocations)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(32, 1), 0);
    EXPECT_EQ(a.Allocate(32, 1), 32);
    EXPECT_EQ(a.Allocate(32, 1), 64);
    EXPECT_EQ(a.Allocate(32, 1), 96);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
}

TEST(RingAllocator, AllocateFullCapacitySeveralAllocationsAligned)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(17, 16), 0);
    EXPECT_EQ(a.Allocate(27, 16), 32);
    EXPECT_EQ(a.Allocate(31, 16), 64);
    EXPECT_EQ(a.Allocate(32, 16), 96);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
}

TEST(RingAllocator, AllocateOverlapOne)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(100, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    EXPECT_EQ(a.Allocate(100, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1002), 1001);
}

TEST(RingAllocator, AllocateOverlapMany)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(10, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    EXPECT_EQ(a.Allocate(10, 1), 10);
    EXPECT_EQ(a.CommitFreeOverlapped(1002), 0);
    EXPECT_EQ(a.Allocate(10, 1), 20);
    EXPECT_EQ(a.CommitFreeOverlapped(1003), 0);
    EXPECT_EQ(a.Allocate(100, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1004), 1003);
}

TEST(RingAllocator, AllocateOverlapFullCapacity)
{
    RingAllocator a(128);
    EXPECT_EQ(a.Allocate(128, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1001), 0);
    EXPECT_EQ(a.Allocate(128, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1002), 1001);
    EXPECT_EQ(a.Allocate(128, 1), 0);
    EXPECT_EQ(a.CommitFreeOverlapped(1003), 1002);
}

