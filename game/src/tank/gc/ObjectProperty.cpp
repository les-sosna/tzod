#include "ObjectProperty.h"
#include <cassert>

ObjectProperty::ObjectProperty(PropertyType type, std::string &&name)
  : _name(std::move(name))
  , _type(type)
  , _value_index(0)
  , _int_min(0)
  , _int_max(0)
{
}

ObjectProperty::PropertyType ObjectProperty::GetType(void) const
{
	return _type;
}

const std::string& ObjectProperty::GetName(void) const
{
	return _name;
}

int ObjectProperty::GetIntValue(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_value;
}

int ObjectProperty::GetIntMin(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_min;
}

int ObjectProperty::GetIntMax(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_max;
}

void ObjectProperty::SetIntValue(int value)
{
	assert(TYPE_INTEGER == _type);
	assert(value >= GetIntMin());
	assert(value <= GetIntMax());
	_int_value = value;
}

void ObjectProperty::SetIntRange(int min, int max)
{
	assert(TYPE_INTEGER == _type);
	_int_min = min;
	_int_max = max;
}

float ObjectProperty::GetFloatValue(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_value;
}

float ObjectProperty::GetFloatMin(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_min;
}

float ObjectProperty::GetFloatMax(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_max;
}

void ObjectProperty::SetFloatValue(float value)
{
	assert(TYPE_FLOAT == _type);
	assert(value >= GetFloatMin());
	assert(value <= GetFloatMax());
	_float_value = value;
}

void ObjectProperty::SetFloatRange(float min, float max)
{
	assert(TYPE_FLOAT == _type);
	_float_min = min;
	_float_max = max;
}

void ObjectProperty::SetStringValue(std::string str)
{
	assert(TYPE_STRING == _type || TYPE_SKIN == _type || TYPE_TEXTURE == _type);
	_str_value = std::move(str);
}

const std::string& ObjectProperty::GetStringValue(void) const
{
	assert(TYPE_STRING == _type || TYPE_SKIN == _type || TYPE_TEXTURE == _type);
	return _str_value;
}

void ObjectProperty::AddItem(std::string str)
{
	assert(TYPE_MULTISTRING == _type);
	_value_set.push_back(std::move(str));
}

const std::string& ObjectProperty::GetListValue(size_t index) const
{
	assert(TYPE_MULTISTRING == _type);
	assert(index < _value_set.size());
	return _value_set[index];
}

size_t ObjectProperty::GetCurrentIndex(void) const
{
	assert(TYPE_MULTISTRING == _type);
	return _value_index;
}

void ObjectProperty::SetCurrentIndex(size_t index)
{
	assert(TYPE_MULTISTRING == _type);
	assert(index < _value_set.size());
	_value_index = index;
}

size_t ObjectProperty::GetListSize(void) const
{
	return _value_set.size();
}


