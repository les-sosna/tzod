#include "inc/gc/TypeSystem.h"
#include <MapFile.h>

#define IMPLEMENT_SELF_REGISTRATION(cls)                           \
	IMPLEMENT_POOLED_ALLOCATION(cls)                               \
	ObjectType cls::_sType = RTTypes::Inst().RegType<cls>(#cls);   \
	bool cls::__registered = cls::__SelfRegister();                \
	bool cls::__SelfRegister()

#define ED_SERVICE(name, desc) RTTypes::Inst().RegisterService<__ThisClass>((name), (desc))

#define ED_MOVING_OBJECT(name, desc, layer, width, height, align, offset, hidden) \
	RTTypes::Inst().RegisterObject<__ThisClass>((name), (desc), (layer), (width), (height), (align), (offset), (hidden))

#define ED_ITEM(name, desc, layer) ED_MOVING_OBJECT(name, desc, layer, 32 /*width*/, 32 /*height*/, WORLD_BLOCK_SIZE/2 /*align*/, 0 /*offset*/, false /*hidden*/)
#define ED_ITEM_HIDDEN(name, desc, layer) ED_MOVING_OBJECT(name, desc, layer, 32 /*width*/, 32 /*height*/, WORLD_BLOCK_SIZE/2 /*align*/, 0 /*offset*/, true /*hidden*/)

#define ED_BLOCK(name, desc, layer) ED_MOVING_OBJECT(name, desc, layer, \
	WORLD_BLOCK_SIZE /*width*/, \
	WORLD_BLOCK_SIZE /*height*/, \
	WORLD_BLOCK_SIZE /*align*/, \
	WORLD_BLOCK_SIZE/2 /*offset*/, \
	false /*hidden*/)

#define ED_TURRET(name, desc) \
	ED_MOVING_OBJECT(name, desc, 0 /*layer*/, WORLD_BLOCK_SIZE*2 /*width*/, WORLD_BLOCK_SIZE*2 /*height*/, WORLD_BLOCK_SIZE /*align*/, WORLD_BLOCK_SIZE /*offset*/, false /*hidden*/)

