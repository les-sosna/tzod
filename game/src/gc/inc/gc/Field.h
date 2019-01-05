#pragma once

#include <math/MyMath.h>
#include <algorithm>
#include <cstdlib>
#include <memory>

class Field;
class GC_RigidBodyStatic;

class FieldCell final
{
public:
	static unsigned int _sessionId;
	unsigned int _mySession = 0xffffffff;
	//-----------------------------
	union
	{
		GC_RigidBodyStatic **_objects = nullptr;
		GC_RigidBodyStatic *_singleObject; // when _objCount==1
	};
	unsigned int _objCount = 0;
	//-----------------------------
	void UpdateProperties();

	int _before; // actual path cost to this node

	uint8_t _prop = 0; // 0 - free, 1 - could be broken, 0xFF - impassable
	int8_t _prev = -1;

public:
	FieldCell() = default;
	FieldCell(const FieldCell &other) = delete;
	FieldCell& operator = (const FieldCell &other) = delete;

	unsigned int GetObjectsCount() const { return _objCount; }
	GC_RigidBodyStatic* GetObject(unsigned int index) const
	{
		return _objCount > 1 ? _objects[index] : _singleObject;
	}

	bool IsChecked() const { return _mySession == _sessionId; }
	void Check()           { _mySession = _sessionId;         }

	void AddObject(GC_RigidBodyStatic *object);
	void RemoveObject(GC_RigidBodyStatic *object);

	unsigned char Properties() const { return _prop; }

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
};

class Field final
{
public:
	static void NewSession() { ++FieldCell::_sessionId; }

	void Resize(int width, int height);
	void ProcessObject(const RectRB &blockBounds, GC_RigidBodyStatic *object, bool add);

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
