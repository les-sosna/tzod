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
	short _x, _y;
	unsigned int _mySession;
	//-----------------------------
	GC_RigidBodyStatic **_ppObjects;
	unsigned int _objCount;
	unsigned char _prop;             // 0 - free, 1 - could be broken, 0xFF - impassable
	//-----------------------------
	void UpdateProperties();
	FieldCell()
        : _mySession(0xffffffff)
        , _ppObjects(nullptr)
        , _objCount(0)
        , _prop(0)     // free
	{}

public:
	const FieldCell *_prevCell;

    int GetObjectsCount() const { return _objCount; }
    GC_RigidBodyStatic* GetObject(int index) const { return _ppObjects[index]; }

    bool IsChecked() const { return _mySession == _sessionId; }
    void Check()           { _mySession = _sessionId;         }

    void UpdatePath(float before, short x, short y)
	{
		int dx = abs(x - GetX());
		int dy = abs(y - GetY());
		float pathAfter = (float) std::max(dx, dy) + (float) std::min(dx, dy) * 0.4142f;
		_before = before;
		_total = before + pathAfter;
	}

	void AddObject(GC_RigidBodyStatic *object);
	void RemoveObject(GC_RigidBodyStatic *object);

    short GetX()               const { return _x;    }
    short GetY()               const { return _y;    }
    unsigned char Properties() const { return _prop; }

    float Total() const { return _total; }
    float Before() const { return _before; }

private:
	float _before;          // path cost to this node
	float _total;           // total path cost estimate

	FieldCell(const FieldCell &other); // no copy
	FieldCell& operator = (const FieldCell &other);
};

class RefFieldCell
{
	FieldCell *_cell;
public:
	RefFieldCell(FieldCell &cell) { _cell = &cell; }
	operator FieldCell& () const { return *_cell; }
	bool operator > (const RefFieldCell &cell) const
	{
		return _cell->Total() > cell._cell->Total();
	}
};


class Field
{
	FieldCell _edgeCell;
	FieldCell **_cells;
	RectRB _bounds;

	void Clear();

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
};
