// JobManager.h


template <class T>
class JobManager
{
	std::list<const T*> _members;
	typename std::list<const T*>::iterator _active;
public:

    ~JobManager()
	{
		_ASSERT(_members.empty());
	}

	void RegisterMember(const T *member)
	{
		_ASSERT(std::find(_members.begin(), _members.end(), member) == _members.end());
		_members.push_back(member);
		if( 1 == _members.size() )
			_active = _members.begin();
	}

	void UnregisterMember(const T *member)
	{
		_ASSERT(std::find(_members.begin(), _members.end(), member) != _members.end());
		if( *_active == member )
			++_active;
		_members.remove(member);
		if( _members.end() == _active )
			_active = _members.begin();
		//	++_active;  -- what is it?
	}

    bool TakeJob(const T *member)
	{
		if( *_active == member )
		{
			if( ++_active == _members.end() )
				_active = _members.begin();
			return true; // allow to work
		}
		return false;
	}
};


// end of file
