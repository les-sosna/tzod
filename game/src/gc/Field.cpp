#include "inc/gc/Field.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/WorldCfg.h"
#include <cassert>

unsigned int FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( unsigned int i = 0; i < _objCount; i++ )
	{
		assert(_ppObjects[i]->GetPassability() > 0);
		if( _ppObjects[i]->GetPassability() > _prop )
			_prop = _ppObjects[i]->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	assert(object);

#ifndef NDEBUG
	for( unsigned int i = 0; i < _objCount; ++i )
	{
		assert(object != _ppObjects[i]);
	}
#endif

	GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount + 1];

	if( _ppObjects )
	{
		assert(_objCount > 0);
        std::memcpy(tmp, _ppObjects, sizeof(GC_RigidBodyStatic*) * _objCount);
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
		_ppObjects = nullptr;
	}
	else
	{
		GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount - 1];
		int j = 0;
		for( unsigned int i = 0; i < _objCount; ++i )
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
	_cells = nullptr;
	_bounds = {};
	_edgeCell._prop = 0xFF;
}

Field::~Field()
{
	Clear();
}

void Field::Clear()
{
	_cells.reset();
	_bounds = {};
}

void Field::Resize(RectRB bounds)
{
	assert(WIDTH(bounds) > 0 && HEIGHT(bounds) > 0);
	Clear();
	_bounds = bounds;
	_cells = std::make_unique<FieldCell[]>(WIDTH(bounds) * HEIGHT(bounds));
	for( int y = bounds.top; y < bounds.bottom; y++ )
	{
		for( int x = bounds.left; x < bounds.right; x++ )
		{
			if(bounds.left == x || bounds.top == y || bounds.right - 1 == x || bounds.bottom - 1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(GC_RigidBodyStatic *object, bool add)
{
	float r = object->GetRadius() / WORLD_BLOCK_SIZE;
	vec2d p = object->GetPos() / WORLD_BLOCK_SIZE;

	assert(r >= 0);

	int xmin = std::min(_bounds.right - 1, std::max(_bounds.left, (int)std::floor(p.x - r + 0.5f)));
	int xmax = std::min(_bounds.right - 1, std::max(_bounds.left, (int)std::floor(p.x + r + 0.5f)));
	int ymin = std::min(_bounds.bottom - 1, std::max(_bounds.top, (int)std::floor(p.y - r + 0.5f)));
	int ymax = std::min(_bounds.bottom - 1, std::max(_bounds.top, (int)std::floor(p.y + r + 0.5f)));

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
                if(_bounds.left == x || _bounds.top == y || _bounds.right-1 == x || _bounds.bottom-1 == y )
                    (*this)(x, y)._prop = 0xFF;
            }
        }
}

FieldCellCompare::FieldCellCompare(const Field &field)
	: _field(field)
{}
