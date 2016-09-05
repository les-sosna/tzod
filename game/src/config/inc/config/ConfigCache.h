// ConfigCache.h
// this file is designed to be included twice

#include "ConfigBase.h"

#include <cassert>


#ifndef CONFIG_CACHE_PASS2

#ifndef CONFIG_CACHE_PASS1_INCLUDED
#define CONFIG_CACHE_PASS1_INCLUDED

namespace config_detail
{
	class ReflectionBase
	{
	public:
		operator ConfVarTable&() { return *_root; }
		ConfVarTable* operator->() { return _root; }
		const ConfVarTable* operator->() const { return _root; }

	protected:
		ReflectionBase(ConfVarTable *root, bool)
			: _root(root)
			, _isOwner(nullptr == root)
		{
			if (_isOwner)
			{
				assert(!_root);
				_root = new ConfVarTable();
			}
		}
		~ReflectionBase()
		{
			if (_isOwner)
			{
				assert(_root);
				delete _root;
			}
		}

		ConfVarTable *_root;

	private:
		bool _isOwner;
	};
}

#define NO_HELP
#define HELPSTRING(h)

#define REFLECTION_BEGIN(ReflectionName)                           \
    REFLECTION_BEGIN_(ReflectionName, config_detail::ReflectionBase)

#define REFLECTION_BEGIN_(ReflectionName, Base)                    \
    class ReflectionName : public Base                             \
    {                                                              \
    public:                                                        \
        ReflectionName(ConfVarTable *bindTo = nullptr, bool freeze = true);


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
#else // CONFIG_CACHE_PASS2
///////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_CACHE_PASS2_INCLUDED
#define CONFIG_CACHE_PASS2_INCLUDED


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

ConfVarNumber& InitIntType(ConfVarNumber &var, const char *type, const char *help = nullptr);
ConfVarNumber& InitIntRange(ConfVarNumber &var, int iMin, int iMax, const char *help = nullptr);
ConfVarNumber& InitFloat(ConfVarNumber &var, const char *help = nullptr);
ConfVarBool& InitBool(ConfVarBool &var, const char *help = nullptr);
ConfVarArray& InitArray(ConfVarArray &var, const char *help = nullptr);
ConfVarTable& InitTable(ConfVarTable &var, const char *help = nullptr);
ConfVarNumber& InitFloatRange(ConfVarNumber &var, float fMin, float fMax, const char *help = nullptr);
ConfVarString& InitStrType(ConfVarString &var, const char *type, const char *help = nullptr);

#define HELPSTRING(h)     ,(h)

#define REFLECTION_BEGIN_(ReflectionName, Base) ReflectionName::ReflectionName(ConfVarTable *root, bool freeze): Base(root, (false
#define REFLECTION_BEGIN(ReflectionName) REFLECTION_BEGIN_(ReflectionName, config_detail::ReflectionBase)

#define VAR_FLOAT( var, def )               )), var( InitFloat(_root->GetNum(#var, (float) (def))
#define VAR_FLOAT_RANGE(var,def,fMin,fMax)  )), var( InitFloatRange(_root->GetNum(#var, (float) (def)), (fMin), (fMax)
#define VAR_INT(   var, def )               )), var( InitIntType(_root->GetNum(#var, (int) (def)), nullptr
#define VAR_INT_TYPE(var, def, type )       )), var( InitIntType(_root->GetNum(#var, (int) (def)), (type)
#define VAR_INT_RANGE(var, def, iMin, iMax) )), var( InitIntRange(_root->GetNum(#var, (int) (def)), (iMin), (iMax)
#define VAR_BOOL(  var, def )               )), var( InitBool(_root->GetBool(#var, (def))
#define VAR_STR(   var, def )               )), var( InitStrType(_root->GetStr(#var, (def)), nullptr
#define VAR_STR_TYPE( var, def, type )      )), var( InitStrType(_root->GetStr(#var, (def)), (type)
#define VAR_ARRAY( var, def )               )), var( InitArray(_root->GetArray(#var, (def))
#define VAR_TABLE( var, def )               )), var( InitTable(_root->GetTable(#var, (def))
#define VAR_REFLECTION( var, type )         )), var( &InitTable(_root->GetTable(#var, nullptr)

#define REFLECTION_END()                    )) { if (freeze) _root->Freeze(true); }

#endif // CONFIG_CACHE_PASS2_INCLUDED
#endif // CONFIG_CACHE_PASS2
