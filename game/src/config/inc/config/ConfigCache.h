// ConfigCache.h
// this file is designed to be included twice

#include "ConfigBase.h"

#include <cassert>


#if CONFIG_CACHE_PASS == 1

// pass one may be included several times as part of different headers so we need guard here
#ifndef CONFIG_CACHE_PASS_ONE_INCLUDED
#define CONFIG_CACHE_PASS_ONE_INCLUDED

namespace config_detail
{
	class UnfreezeHelper
	{
	public:
		UnfreezeHelper(ConfVarTable *table)
			: _table(table)
		{
			assert(_table->IsFrozen());
			_table->Freeze(false);
		}
		~UnfreezeHelper()
		{
			assert(!_table->IsFrozen());
			_table->Freeze(true);
		}
		ConfVarTable* operator -> () const
		{
			return _table;
		}
	private:
		ConfVarTable * const _table;
	};

	class ReflectionBase
	{
	public:
		ConfVarTable* GetRoot() const { return _root; }

	protected:
		ReflectionBase(ConfVarTable *root)
			: _root(root)
			, _owner(nullptr == root)
		{
			if( _owner )
			{
				assert(!_root);
				_root = new ConfVarTable();
			}
			_root->Freeze(true);
		}
		~ReflectionBase()
		{
			if( _owner )
			{
				assert(_root);
				delete _root;
			}
		}

	private:
		ConfVarTable  *_root;
		bool _owner;
	};
}

#define NO_HELP
#define HELPSTRING(h)

#define REFLECTION_BEGIN(ReflectionName)                           \
    class ReflectionName : protected config_detail::ReflectionBase \
    {                                                              \
    public:                                                        \
        ReflectionName(ConfVarTable *bindTo = nullptr);               \
        const config_detail::ReflectionBase* operator -> ()        \
        {                                                          \
            return this;                                           \
        }

#define REFLECTION_BEGIN_(ReflectionName, Base)                    \
    class ReflectionName : public Base                             \
    {                                                              \
    public:                                                        \
        ReflectionName(ConfVarTable *bindTo = nullptr);


 #define VAR_FLOAT( var, def )                   ConfVarNumber &var;
 #define VAR_FLOAT_RANGE( var, def, fMin, fMax ) ConfVarNumber &var;
 #define VAR_INT(   var, def )                   ConfVarNumber &var;
 #define VAR_INT_RANGE( var, def, iMin, iMax )   ConfVarNumber &var;
 #define VAR_INT_TYPE( var, def, type )          ConfVarNumber &var;
 #define VAR_BOOL(  var, def )                   ConfVarBool   &var;
 #define VAR_STR(   var, def )                   ConfVarString &var;
 #define VAR_STR_TYPE( var, def, type )          ConfVarString &var;
 #define VAR_ARRAY( var, def )                   ConfVarArray  &var;
 #define VAR_TABLE( var, def )                   ConfVarTable  &var;
 #define VAR_REFLECTION( var, type )             type var;

#define REFLECTION_END()  };
#endif //CONFIG_CACHE_PASS_ONE_INCLUDED

///////////////////////////////////////////////////////////////////////////////
#elif CONFIG_CACHE_PASS == 2

#undef NO_HELP
#undef HELPSTRING

#undef REFLECTION_BEGIN
#undef REFLECTION_BEGIN_
 #undef VAR_FLOAT
 #undef VAR_FLOAT_RANGE
 #undef VAR_INT
 #undef VAR_INT_RANGE
 #undef VAR_INT_TYPE
 #undef VAR_BOOL
 #undef VAR_STR
 #undef VAR_STR_TYPE
 #undef VAR_ARRAY
 #undef VAR_TABLE
 #undef VAR_REFLECTION
#undef REFLECTION_END

static ConfVarNumber& InitIntType(ConfVarNumber *var, const char *type, const char *help = nullptr)
{
//	if( type )
//	{
//		ConfVarNumber::NumberMeta meta;
//		meta.type = type;
//		var->SetMeta(&meta);
//	}
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarNumber& InitIntRange(ConfVarNumber *var, int iMin, int iMax, const char *help = nullptr)
{
//	ConfVarNumber::NumberMeta meta;
//	meta.type = "integer";
//	meta.fMin = (float) iMin;
//	meta.fMax = (float) iMax;
//	var->SetMeta(&meta);
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarNumber& InitFloat(ConfVarNumber *var, const char *help = nullptr)
{
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarBool& InitBool(ConfVarBool *var, const char *help = nullptr)
{
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarArray& InitArray(ConfVarArray *var, const char *help = nullptr)
{
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarTable& InitTable(ConfVarTable *var, const char *help = nullptr)
{
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarNumber& InitFloatRange(ConfVarNumber *var, float fMin, float fMax, const char *help = nullptr)
{
//	ConfVarNumber::NumberMeta meta;
//	meta.type = "float";
//	meta.fMin = fMin;
//	meta.fMax = fMax;
//	var->SetMeta(&meta);
//	if( help ) var->SetHelpString(help);
	return *var;
}

static ConfVarString& InitStrType(ConfVarString *var, const char *type, const char *help = nullptr)
{
//	if( type )
//	{
//		ConfVarString::StringMeta meta;
//		meta.type = type;
//		var->SetMeta(&meta);
//	}
//	if( help ) var->SetHelpString(help);
	return *var;
}


#define HELPSTRING(h)     ,(h)

#define REFLECTION_BEGIN_(ReflectionName, Base) ReflectionName::ReflectionName(ConfVarTable *root): Base((root
#define REFLECTION_BEGIN(ReflectionName) REFLECTION_BEGIN_(ReflectionName, config_detail::ReflectionBase)

#define VAR_FLOAT( var, def )               )), var( InitFloat(config_detail::UnfreezeHelper(GetRoot())->GetNum(#var, (float) (def))
#define VAR_FLOAT_RANGE(var,def,fMin,fMax)  )), var( InitFloatRange(config_detail::UnfreezeHelper(GetRoot())->GetNum(#var, (float) (def)), (fMin), (fMax)
#define VAR_INT(   var, def )               )), var( InitIntType(config_detail::UnfreezeHelper(GetRoot())->GetNum(#var, (int) (def)), nullptr
#define VAR_INT_TYPE(var, def, type )       )), var( InitIntType(config_detail::UnfreezeHelper(GetRoot())->GetNum(#var, (int) (def)), (type)
#define VAR_INT_RANGE(var, def, iMin, iMax) )), var( InitIntRange(config_detail::UnfreezeHelper(GetRoot())->GetNum(#var, (int) (def)), (iMin), (iMax)
#define VAR_BOOL(  var, def )               )), var( InitBool(config_detail::UnfreezeHelper(GetRoot())->GetBool(#var, (def))
#define VAR_STR(   var, def )               )), var( InitStrType(config_detail::UnfreezeHelper(GetRoot())->GetStr(#var, (def)), nullptr
#define VAR_STR_TYPE( var, def, type )      )), var( InitStrType(config_detail::UnfreezeHelper(GetRoot())->GetStr(#var, (def)), (type)
#define VAR_ARRAY( var, def )               )), var( InitArray(config_detail::UnfreezeHelper(GetRoot())->GetArray(#var, (def))
#define VAR_TABLE( var, def )               )), var( InitTable(config_detail::UnfreezeHelper(GetRoot())->GetTable(#var, (def))
#define VAR_REFLECTION( var, type )         )), var( &InitTable(config_detail::UnfreezeHelper(GetRoot())->GetTable(#var, nullptr)

#define REFLECTION_END()                    )) {}

#else
 #error define CONFIG_CACHE_PASS as 1 or 2 before including this file
#endif

#undef CONFIG_CACHE_PASS


// end of file
