#include "inc/gc/Field.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/WorldCfg.h"
#include <cassert>

unsigned int FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( unsigned int i = 0; i < GetObjectsCount(); i++ )
	{
		assert(GetObject(i)->GetPassability() > 0);
		if(GetObject(i)->GetPassability() > _prop )
			_prop = GetObject(i)->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	assert(object);

#ifndef NDEBUG
	for( unsigned int i = 0; i < GetObjectsCount(); ++i )
	{
		assert(object != GetObject(i));
	}
#endif

	if (_objCount > 0)
	{
		std::unique_ptr<GC_RigidBodyStatic*[]> tmp(new GC_RigidBodyStatic*[_objCount + 1]);
		if (_objCount == 1)
		{
			tmp[0] = _singleObject;
		}
		else
		{
			std::memcpy(tmp.get(), _objects, _objCount * sizeof(GC_RigidBodyStatic*));
			delete[] _objects;
		}
		tmp[_objCount] = object;
		_objects = tmp.release();
	}
	else
	{
		_singleObject = object;
	}

	_objCount++;

	UpdateProperties();
}

void FieldCell::RemoveObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount > 0);

	if( 1 == _objCount )
	{
		assert(object == _singleObject);
		_singleObject = nullptr;
	}
	else if (2 == _objCount)
	{
		GC_RigidBodyStatic *tmp = _objects[0] == object ? _objects[1] : _objects[0];
		delete[] _objects;
		_singleObject = tmp;
	}
	else
	{
		auto tmp = std::make_unique<GC_RigidBodyStatic*[]>(_objCount - 1);
		int j = 0;
		for( unsigned int i = 0; i < _objCount; ++i )
		{
			if( object != _objects[i] )
				tmp[j++] = _objects[i];
		}
		assert(j == _objCount - 1);
		delete[] _objects;
		_objects = tmp.release();
	}
	_objCount--;
	UpdateProperties();
}

////////////////////////////////////////////////////////////

Field::Field()
{
	_edgeCell._prop = 0xFF;
}

void Field::Resize(RectRB bounds)
{
	assert(WIDTH(bounds) > 0 && HEIGHT(bounds) > 0);
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

	for (int x = xmin; x <= xmax; x++)
	{
		for (int y = ymin; y <= ymax; y++)
		{
			if (add)
			{
				(*this)(x, y).AddObject(object);
			}
			else
			{
				(*this)(x, y).RemoveObject(object);
				if (_bounds.left == x || _bounds.top == y || _bounds.right - 1 == x || _bounds.bottom - 1 == y)
					(*this)(x, y)._prop = 0xFF;
			}
		}
	}
}
