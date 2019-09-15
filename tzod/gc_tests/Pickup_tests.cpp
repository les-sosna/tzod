#include <gc/Weapons.h>
#include <gc/World.h>
#include <gtest/gtest.h>

TEST(Pickup, CanRespawn)
{
	World world({ 0, 0, 16, 16 }, false /*initField*/);
	auto &weapon = world.New<GC_Weap_Cannon>(vec2d{});
	weapon.Disappear(world);
	world.Step(10);
	EXPECT_TRUE(weapon.GetVisible());
}
