#pragma once
#include "ObjectView.h"
#include <gc/Actor.h>
#include <vector>
#include <memory>
#include <map>

struct ObjectView
{
	std::unique_ptr<ObjectZFunc> zfunc;
	std::unique_ptr<ObjectRFunc> rfunc;
};

class ObjectViewsSelectorBuilder
{
public:
	template <class T>
	void AddView(std::unique_ptr<ObjectZFunc> zfunc, std::unique_ptr<ObjectRFunc> rfunc)
	{
		_type2views.emplace(T::GetTypeStatic(), ObjectView{ std::move(zfunc), std::move(rfunc) });
	}

private:
	friend class ObjectViewsSelector;
	std::multimap<ObjectType, ObjectView> _type2views;
};

typedef std::pair<const ObjectView*, const ObjectView*> ViewCollection;
inline const ObjectView* begin(const ViewCollection &v) { return v.first; }
inline const ObjectView* end(const ViewCollection &v) { return v.second; }

class ObjectViewsSelector
{
public:
	ObjectViewsSelector(ObjectViewsSelectorBuilder &&builder);
	inline ViewCollection GetViews(const GC_Actor &actor) const
	{
		ObjectType type = actor.GetType();
		return { _typeToFirstView[type], _typeToFirstView[type + 1] };
	}
private:
	std::vector<ObjectView> _views;
	std::vector<const ObjectView*> _typeToFirstView;
};
