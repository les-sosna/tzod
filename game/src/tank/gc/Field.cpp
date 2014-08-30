// Field.cpp

#include "Field.h"
#include "constants.h"
#include "gc/RigidBody.h"
#include "core/debug.h"
#include <cassert>

unsigned long FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( int i = 0; i < _objCount; i++ )
	{
		assert(_ppObjects[i]->GetPassability() > 0);
		if( _ppObjects[i]->GetPassability() > _prop )
			_prop = _ppObjects[i]->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount < 255);
    
#ifdef _DEBUG
	for( int i = 0; i < _objCount; ++i )
	{
		assert(object != _ppObjects[i]);
	}
#endif
    
	GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount + 1];
    
	if( _ppObjects )
	{
		assert(_objCount > 0);
		memcpy(tmp, _ppObjects, sizeof(GC_RigidBodyStatic*) * _objCount);
		delete[] _ppObjects;
	}
    
	_ppObjects = tmp;
	_ppObjects[_objCount++] = object;
    
	UpdateProperties();
}

void FieldCell::RemoveObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount > 0);
    
	if( 1 == _objCount )
	{
		assert(object == _ppObjects[0]);
		_objCount = 0;
		delete[] _ppObjects;
		_ppObjects = NULL;
	}
	else
	{
		GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount - 1];
		int j = 0;
		for( int i = 0; i < _objCount; ++i )
		{
			if( object == _ppObjects[i] ) continue;
			tmp[j++] = _ppObjects[i];
		}
		_objCount--;
		assert(j == _objCount);
		delete[] _ppObjects;
		_ppObjects = tmp;
	}
    
	UpdateProperties();
}

////////////////////////////////////////////////////////////

Field::Field()
{
	_cells = NULL;
	_cx    = 0;
	_cy    = 0;
    
	_edgeCell._prop = 0xFF;
	_edgeCell._x    = -1;
	_edgeCell._y    = -1;
}

Field::~Field()
{
	Clear();
}

void Field::Clear()
{
	if( _cells )
	{
		for( int i = 0; i < _cy; i++ )
			delete[] _cells[i];
		delete[] _cells;
		//-----------
		_cells = NULL;
		_cx    = 0;
		_cy    = 0;
	}
	assert(0 == _cx && 0 == _cy);
}

void Field::Resize(int cx, int cy)
{
	assert(cx > 0 && cy > 0);
	Clear();
	_cx = cx;
	_cy = cy;
	_cells = new FieldCell* [_cy];
	for( int y = 0; y < _cy; y++ )
	{
		_cells[y] = new FieldCell[_cx];
		for( int x = 0; x < _cx; x++ )
		{
			(*this)(x, y)._x = x;
			(*this)(x, y)._y = y;
			if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(GC_RigidBodyStatic *object, bool add)
{
	float r = object->GetRadius() / CELL_SIZE;
	vec2d p = object->GetPos() / CELL_SIZE;
    
	assert(r >= 0);
    
	int xmin = std::max(0,       int(p.x - r + 0.5f));
	int xmax = std::min(_cx - 1, int(p.x + r + 0.5f));
	int ymin = std::max(0,       int(p.y - r + 0.5f));
	int ymax = std::min(_cy - 1, int(p.y + r + 0.5f));
    
	for( int x = xmin; x <= xmax; x++ )
        for( int y = ymin; y <= ymax; y++ )
        {
            if( add )
            {
                (*this)(x, y).AddObject(object);
            }
            else
            {
                (*this)(x, y).RemoveObject(object);
                if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
                    (*this)(x, y)._prop = 0xFF;
            }
        }
}

#ifdef _DEBUG
FieldCell& Field::operator() (int x, int y)
{
	assert(NULL != _cells);
	return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
}

void Field::Dump()
{
	TRACE("==== Field dump ====");
    
	std::string buf;
	for( int y = 0; y < _cy; y++ )
	{
		for( int x = 0; x < _cx; x++ )
		{
			switch( (*this)(x, y).Properties() )
			{
                case 0:
                    buf.push_back(' ');
                    break;
                case 1:
                    buf.push_back('-');
                    break;
                case 0xFF:
                    buf.push_back('#');
                    break;
			}
		}
		TRACE("%s", buf.c_str());
		buf.clear();
	}
    
	TRACE("=== end of dump ====");
}

#endif