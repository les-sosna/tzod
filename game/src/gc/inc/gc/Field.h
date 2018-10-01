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

	float _before; // actual path cost to this node
	float _total; // total path cost estimate

	uint8_t _prop = 0; // 0 - free, 1 - could be broken, 0xFF - impassable
	int8_t _stepX = 0;
	int8_t _stepY = 0;

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

	float Total() const { return _total; }
	float Before() const { return _before; }
};

struct RefFieldCell
{
	int x : 16;
	int y : 16;

	bool operator==(const RefFieldCell &other) const
	{
		return x == other.x && y == other.y;
	}
};

class Field final
{
public:
	static void NewSession() { ++FieldCell::_sessionId; }

	Field();

	void Resize(RectRB bounds);
	void ProcessObject(GC_RigidBodyStatic *object, bool add);

	RectRB GetBounds() const { return _bounds; }

	const FieldCell& operator() (int x, int y) const
	{
		if (PtInRect(_bounds, x, y))
		{
			size_t offset = (x - _bounds.left) + (y - _bounds.top) * WIDTH(_bounds);
			return _cells.get()[offset];
		}
		else
		{
			return _edgeCell;
		}
	}

	FieldCell& operator() (int x, int y)
	{
		return const_cast<FieldCell&>(static_cast<const Field*>(this)->operator()(x, y));
	}

private:
	FieldCell _edgeCell;
	std::unique_ptr<FieldCell[]> _cells;
	RectRB _bounds = {};
};

class FieldCellCompare final
{
public:
	FieldCellCompare(const Field &field);

	bool operator()(const RefFieldCell &a, const RefFieldCell &b) const
	{
		return _field(a.x, a.y).Total() > _field(b.x, b.y).Total();
	}

private:
	const Field &_field;
};

