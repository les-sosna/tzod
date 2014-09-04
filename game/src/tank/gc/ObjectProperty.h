#pragma once

#include <string>
#include <vector>

class ObjectProperty
{
public:
	enum PropertyType
	{
		TYPE_INTEGER,
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_SKIN, // string value
		TYPE_TEXTURE, // string value
		TYPE_MULTISTRING,
	};

private:
	std::string               _name;
	PropertyType              _type;
	std::string               _str_value;
	std::vector<std::string>  _value_set;
	union {
		size_t             _value_index;
		int                _int_value;
		float              _float_value;
	};
	union {
		int                _int_min;
		float              _float_min;
	};
	union {
		int                _int_max;
		float              _float_max;
	};

public:
	ObjectProperty(PropertyType type, std::string &&name);

	const std::string& GetName(void) const;
	PropertyType GetType(void) const;


	//
	// TYPE_INTEGER
	//
	int  GetIntValue(void) const;
	int  GetIntMin(void) const;
	int  GetIntMax(void) const;
	void SetIntValue(int value);
	void SetIntRange(int min, int max);

	//
	// TYPE_FLOAT
	//
	float GetFloatValue(void) const;
	float GetFloatMin(void) const;
	float GetFloatMax(void) const;
	void  SetFloatValue(float value);
	void  SetFloatRange(float min, float max);


	//
	// TYPE_STRING, TYPE_SKIN
	//
	void SetStringValue(std::string str);
	const std::string& GetStringValue(void) const;


	//
	// TYPE_MULTISTRING
	//
	void   AddItem(std::string str);
	size_t GetCurrentIndex(void) const;
	void   SetCurrentIndex(size_t index);
	size_t GetListSize(void) const;
	const std::string& GetListValue(size_t index) const;
};
