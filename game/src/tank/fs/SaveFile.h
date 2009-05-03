// SaveFile.h


class SaveFile
{
	typedef std::map<void*, size_t> PtrToIndex;
	typedef std::map<size_t, void*> IndexToPtr;

	PtrToIndex _ptrToIndex;
	IndexToPtr _indexToPtr;

	std::list<SafePtr<void>*>    _refs;

public:
	HANDLE _file;
	bool   _load;

	bool loading() const
	{
		return _load;
	}


	void Serialize(string_t &str);

	template<class T>
	void Serialize(T &obj);
	template<class T>
	void Serialize(const T &obj);

	template<class T>
	void Serialize(SafePtr<T> &ptr);
	template<class T>
	void Serialize(const SafePtr<T> &ptr);

	template<class T>
	void SerializeArray(T *p, size_t count);

	bool RestoreAllLinks();
	void RegPointer(void *ptr, size_t index);

private:
	template<class T>
	void Serialize(T *obj) {assert(FALSE);} // you are not allowed to serialize raw pointers
};

template<class T>
void SaveFile::Serialize(T &obj)
{
	assert(0 != strcmp(typeid(obj).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(obj).raw_name(), "SafePtr"));
	DWORD bytes;
	if( loading() )
		ReadFile(_file, &obj, sizeof(T), &bytes, NULL);
	else
		WriteFile(_file, &obj, sizeof(T), &bytes, NULL);
}

template<class T>
void SaveFile::Serialize(const T &obj)
{
	assert(!loading());
	assert(0 != strcmp(typeid(obj).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(obj).raw_name(), "SafePtr"));
	DWORD bytes;
	WriteFile(_file, &obj, sizeof(T), &bytes, NULL);
}

template<class T>
void SaveFile::Serialize(SafePtr<T> &ptr)
{
	DWORD_PTR id;
	if( loading() )
	{
		Serialize(id);
		SetRawPtr(ptr, reinterpret_cast<T*>(id));
		_refs.push_back(reinterpret_cast<SafePtr<void>*>(&ptr));
	}
	else
	{
		if( !ptr )
		{
			id = 0;
		}
		else
		{
			if( _ptrToIndex.count(GetRawPtr(ptr)) )
			{
				id = _ptrToIndex[GetRawPtr(ptr)];
			}
			else
			{
				id = _ptrToIndex.size() + 1;
				_ptrToIndex[GetRawPtr(ptr)] = id;
			}
		}
		Serialize(id);
	}
}

template<class T>
void SaveFile::Serialize(const SafePtr<T> &ptr)
{
	assert(!loading());
	DWORD_PTR id;
	if( !ptr )
	{
		id = 0;
	}
	else
	{
		if( _ptrToIndex.count(GetRawPtr(ptr)) )
		{
			id = _ptrToIndex[GetRawPtr(ptr)];
		}
		else
		{
			id = _ptrToIndex.size() + 1;
			_ptrToIndex[GetRawPtr(ptr)] = id;
		}
	}
	Serialize(id);
}

template<class T>
void SaveFile::SerializeArray(T *p, size_t count)
{
	assert(0 != strcmp(typeid(T).raw_name(), typeid(string_t).raw_name()));
	assert(NULL == strstr(typeid(T).raw_name(), "SafePtr"));
	DWORD bytes;
	if( loading() )
		ReadFile(_file, p, sizeof(T)*count, &bytes, NULL);
	else
		WriteFile(_file, p, sizeof(T)*count, &bytes, NULL);
}

// end of file
