#include "ObjectViewsSelector.h"
#include "ObjectView.h"
#include "gc/Actor.h"

ObjectView::ObjectView(std::unique_ptr<ObjectZFunc> zf, std::unique_ptr<ObjectRFunc> rf)
	: zfunc(std::move(zf))
	, rfunc(std::move(rf))
{}

const ObjectViewsSelector::ViewCollection* ObjectViewsSelector::GetViews(const GC_Actor &actor) const
{
	ObjectType type = actor.GetType();
	return (type < _type2views.size() && !_type2views[type].empty()) ? &_type2views[type] : nullptr;
}
