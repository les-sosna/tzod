#pragma once

#include <math/MyMath.h>
#include <algorithm>
#include <cstdlib>

class Field;
class GC_RigidBodyStatic;

class FieldCell
{
public:
	static unsigned int _sessionId;
	unsigned int _mySession = 0xffffffff;
	//-----------------------------
	GC_RigidBodyStatic **_ppObjects = nullptr;
	unsigned int _objCount = 0;
	//-----------------------------
	void UpdateProperties();

	float _before; // actual path cost to this node
	float _total; // total path cost estimate

	unsigned char _prop = 0; // 0 - free, 1 - could be broken, 0xFF - impassable
	int8_t _stepX = 0;
	int8_t _stepY = 0;

public:
	FieldCell() = default;
	FieldCell(const FieldCell &other) = delete;
	FieldCell& operator = (const FieldCell &other) = delete;

	int GetObjectsCount() const { return _objCount; }
	GC_RigidBodyStatic* GetObject(int index) const { return _ppObjects[index]; }

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

class Field
{
public:
	static void NewSession() { ++FieldCell::_sessionId; }

	Field();
	~Field();

	void Resize(RectRB bounds);
	void ProcessObject(GC_RigidBodyStatic *object, bool add);

	RectRB GetBounds() const { return _bounds; }

	FieldCell& operator() (int x, int y)
	{
		return PtInRect(_bounds, x, y) ? _cells[y - _bounds.top][x - _bounds.left] : _edgeCell;
	}

	const FieldCell& operator() (int x, int y) const
	{
		return PtInRect(_bounds, x, y) ? _cells[y - _bounds.top][x - _bounds.left] : _edgeCell;
	}

private:
	FieldCell _edgeCell;
	FieldCell **_cells;
	RectRB _bounds;

	void Clear();
};

class FieldCellCompare
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

