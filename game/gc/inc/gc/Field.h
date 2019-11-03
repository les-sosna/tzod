#pragma once
#include "Object.h"
#include <math/MyMath.h>
#include <algorithm>
#include <cstdlib>
#include <memory>

class FieldCell final
{
public:
	static unsigned int _sessionId;
	unsigned int _mySession = 0xffffffff;
	//-----------------------------
	union X
	{
		ObjectList::id_type* _objects;
		ObjectList::id_type _singleObject; // when _objCount==1

		X() {}
	} _storage;
	unsigned int _objCount = 0;
	//-----------------------------
	void UpdateProperties(const World &world);

	int _before; // actual path cost to this node

	// each bit describes a separate obstacle group: 0 - passable, 1 - occupied
	uint8_t _obstacleFlags = 0;
	int8_t _prev = -1;

public:
	FieldCell() = default;
	FieldCell(const FieldCell &other) = delete;
	FieldCell& operator = (const FieldCell &other) = delete;

	unsigned int GetObjectsCount() const { return _objCount; }
	ObjectList::id_type GetObject(unsigned int index) const
	{
		return _objCount > 1 ? _storage._objects[index] : _storage._singleObject;
	}

	bool IsChecked() const { return _mySession == _sessionId; }
	void Check()           { _mySession = _sessionId;         }

	void AddObject(const World& world, ObjectList::id_type object);
	void RemoveObject(const World& world, ObjectList::id_type object);

	uint8_t ObstacleFlags() const { return _obstacleFlags; }

	int Before() const { return _before; }
};

struct RefFieldCell
{
	int x : 16;
	int y : 16;

	bool operator==(RefFieldCell other) const
	{
		return x == other.x && y == other.y;
	}
	bool operator!=(RefFieldCell other) const
	{
		return x != other.x || y != other.y;
	}
};

class GC_RigidBodyStatic;

class Field final
{
public:
	static void NewSession() { ++FieldCell::_sessionId; }

	void Resize(int width, int height);
	void ProcessObject(const World& world, GC_RigidBodyStatic *object, bool add);

	const FieldCell& operator() (int x, int y) const
	{
		assert(x >= 0 && x < _width && y >= 0 && y < _height);
		return _cells.get()[x + y * _width];
	}

	FieldCell& operator() (int x, int y)
	{
		return const_cast<FieldCell&>(static_cast<const Field*>(this)->operator()(x, y));
	}

private:
	std::unique_ptr<FieldCell[]> _cells;
	int _width = 0;
	int _height = 0;
};
