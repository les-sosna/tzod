#pragma once

#include <string>
#include <string_view>
#include <vector>

class ObjectProperty final
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

	ObjectProperty(PropertyType type, std::string name);

	std::string_view GetName() const;
	PropertyType GetType() const;


	//
	// TYPE_INTEGER
	//
	int  GetIntValue() const;
	int  GetIntMin() const;
	int  GetIntMax() const;
	void SetIntValue(int value);
	void SetIntRange(int min, int max);

	//
	// TYPE_FLOAT
	//
	float GetFloatValue() const;
	float GetFloatMin() const;
	float GetFloatMax() const;
	void  SetFloatValue(float value);
	void  SetFloatRange(float min, float max);


	//
	// TYPE_STRING, TYPE_SKIN
	//
	void SetStringValue(std::string str);
	std::string_view GetStringValue() const;


	//
	// TYPE_MULTISTRING
	//
	void   AddItem(std::string str);
	size_t GetCurrentIndex() const;
	void   SetCurrentIndex(size_t index);
	size_t GetListSize() const;
	std::string_view GetListValue(size_t index) const;

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
};
