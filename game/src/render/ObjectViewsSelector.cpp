#include "inc/render/ObjectViewsSelector.h"
#include "inc/render/ObjectView.h"
#include <gc/Actor.h>

ObjectViewsSelector::ObjectViewsSelector(ObjectViewsSelectorBuilder &&builder)
{
	_views.reserve(builder._type2views.size());
	_typeToFirstView.reserve(builder._type2views.rbegin()->first + 1); // extra slot for the end iterator

	ObjectType currentType = 0;
	for (auto& t2v: builder._type2views)
	{
		_views.push_back(std::move(t2v.second));
		// fill possible gaps between types
		for (; currentType <= t2v.first; currentType++)
		{
			_typeToFirstView.push_back(&_views.back());
		}
	}
	_typeToFirstView.emplace_back(&_views.back() + 1); // last type end iterator
}
