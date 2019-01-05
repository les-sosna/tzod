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

void Field::Resize(int width, int height)
{
	assert(width > 0 && height > 0);
	_cells = std::make_unique<FieldCell[]>(width * height);
	_width = width;
	_height = height;
	for( int y = 0; y < height; y++ )
	{
		for( int x = 0; x < width; x++ )
		{
			if( 0 == x || 0 == y || _width - 1 == x || _height - 1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(const RectRB &blockBounds, GC_RigidBodyStatic *object, bool add)
{
	float r = object->GetRadius() / WORLD_BLOCK_SIZE;
	vec2d p = object->GetPos() / WORLD_BLOCK_SIZE;

	assert(WIDTH(blockBounds) + 1 == _width && HEIGHT(blockBounds) + 1 == _height);
	assert(r >= 0);

	int xmin = std::min(blockBounds.right, std::max(blockBounds.left, (int)std::floor(p.x - r + 0.5f)));
	int xmax = std::min(blockBounds.right, std::max(blockBounds.left, (int)std::floor(p.x + r + 0.5f)));
	int ymin = std::min(blockBounds.bottom, std::max(blockBounds.top, (int)std::floor(p.y - r + 0.5f)));
	int ymax = std::min(blockBounds.bottom, std::max(blockBounds.top, (int)std::floor(p.y + r + 0.5f)));

	for (int x = xmin; x <= xmax; x++)
	{
		for (int y = ymin; y <= ymax; y++)
		{
			if (add)
			{
				(*this)(x - blockBounds.left, y - blockBounds.top).AddObject(object);
			}
			else
			{
				(*this)(x - blockBounds.left, y - blockBounds.top).RemoveObject(object);
				if (blockBounds.left == x || blockBounds.top == y || blockBounds.right == x || blockBounds.bottom == y)
					(*this)(x - blockBounds.left, y - blockBounds.top)._prop = 0xFF;
			}
		}
	}
}
