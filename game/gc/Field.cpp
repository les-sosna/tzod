#include "inc/gc/Field.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include <cassert>

unsigned int FieldCell::_sessionId;

void FieldCell::UpdateProperties(const World& world)
{
	_obstacleFlags = 0;
	for( unsigned int i = 0; i < GetObjectsCount(); i++ )
	{
		auto object = static_cast<GC_RigidBodyStatic*>(world.GetList(GlobalListID::LIST_objects).at(GetObject(i)));
		assert(object->GetObstacleFlags());
		_obstacleFlags |= object->GetObstacleFlags();
	}
}

void FieldCell::AddObject(const World& world, ObjectList::id_type object)
{
#ifndef NDEBUG
	for( unsigned int i = 0; i < GetObjectsCount(); ++i )
	{
		assert(object != GetObject(i));
	}
#endif

	if (_objCount > 0)
	{
		std::unique_ptr<ObjectList::id_type[]> tmp(new ObjectList::id_type[_objCount + 1]);
		if (_objCount == 1)
		{
			tmp[0] = _storage._singleObject;
		}
		else
		{
			std::memcpy(tmp.get(), _storage._objects, _objCount * sizeof(ObjectList::id_type));
			delete[] _storage._objects;
		}
		tmp[_objCount] = object;
		_storage._objects = tmp.release();
	}
	else
	{
		_storage._singleObject = object;
	}

	_objCount++;

	UpdateProperties(world);
}

void FieldCell::RemoveObject(const World& world, ObjectList::id_type object)
{
	assert(_objCount > 0);

	if( 1 == _objCount )
	{
		assert(object == _storage._singleObject);
		_storage._singleObject = ObjectList::id_type();
	}
	else if (2 == _objCount)
	{
		ObjectList::id_type tmp = _storage._objects[0] == object ? _storage._objects[1] : _storage._objects[0];
		delete[] _storage._objects;
		_storage._singleObject = tmp;
	}
	else
	{
		auto tmp = std::make_unique<ObjectList::id_type[]>(_objCount - 1);
		int j = 0;
		for( unsigned int i = 0; i < _objCount; ++i )
		{
			if( object != _storage._objects[i] )
				tmp[j++] = _storage._objects[i];
		}
		assert(j == _objCount - 1);
		delete[] _storage._objects;
		_storage._objects = tmp.release();
	}
	_objCount--;
	UpdateProperties(world);
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
				(*this)(x, y)._obstacleFlags = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(const World& world, GC_RigidBodyStatic *object, bool add)
{
	RectRB blockBounds = world.GetBlockBounds();
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
				(*this)(x - blockBounds.left, y - blockBounds.top).AddObject(world, object->GetId());
			}
			else
			{
				(*this)(x - blockBounds.left, y - blockBounds.top).RemoveObject(world, object->GetId());
				if (blockBounds.left == x || blockBounds.top == y || blockBounds.right == x || blockBounds.bottom == y)
					(*this)(x - blockBounds.left, y - blockBounds.top)._obstacleFlags = 0xFF;
			}
		}
	}
}
