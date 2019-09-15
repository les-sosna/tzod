#pragma once

class World;

struct GI_NeighborAware
{
	virtual int GetNeighbors(const World &world) const = 0;
};
