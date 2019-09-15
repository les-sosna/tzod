#include <fsmem/FileSystemMemory.h>
#include <gc/SpawnPoint.h>
#include <gc/SaveFile.h>
#include <gc/World.h>
#include <gtest/gtest.h>

TEST(Serialization, CanSerializeEmptyWorld)
{
	FS::MemoryStream stream;
	{
		SaveFile f(stream, false /*loading*/);
		World world({ 0, 0, 16, 16 }, false /*initField*/);
		world.Serialize(f);
		EXPECT_EQ(10, stream.Tell());
	}

	stream.Seek(0, SEEK_SET);
	{
		World world({ 0, 0, 16, 16 }, false /*initField*/); // FIXME: restore bounds from file
		SaveFile f(stream, true /*loading*/);
		world.Serialize(f);
		EXPECT_EQ(10, stream.Tell());
	}
}

TEST(Serialization, CanSerializeNamedObject)
{
	FS::MemoryStream stream;
	{
		World world({ 0, 0, 16, 16 }, false /*initField*/);
		auto &obj = world.New<GC_SpawnPoint>(vec2d{});
		obj.SetName(world, "abc");

		SaveFile f(stream, false /*loading*/);
		world.Serialize(f);
	}

	auto size = stream.Tell();
	stream.Seek(0, SEEK_SET);
	{
		World world({ 0, 0, 16, 16 }, false /*initField*/);
		SaveFile f(stream, true /*loading*/);
		world.Serialize(f);
		EXPECT_EQ(size, stream.Tell());
		EXPECT_TRUE(world.FindObject("abc") != nullptr);
	}
}



