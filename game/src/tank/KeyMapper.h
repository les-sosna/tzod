// KeyMapper.h

#pragma once

class KeyMapper
{
	std::map<string_t, int> _name2code;
	std::map<int, string_t> _code2name;

	void Pair(const char *name, int code);

public:
	KeyMapper();
	~KeyMapper();

	string_t GetName(int code) const;
	int GetCode(const string_t &name) const;
};

// end of file
