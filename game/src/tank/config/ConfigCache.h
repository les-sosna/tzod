// ConfigCache.h
// this file is designed to be included twice

#include "ConfigBase.h"

#if CONFIG_CACHE_PASS == 1

#define CONFIG_BEGIN(StructName)                    \
    struct StructName                               \
    {                                               \
		struct MyAccessorBase {};                   \
		struct MyAccessor : public MyAccessorBase   \
		{                                           \
			MyAccessor(ConfVarTable *root);


#define CONFIG_END(StructName, VarName)             \
		};                                          \
		static ConfVarTable* GetRoot()              \
		{                                           \
			static ConfVarTable root;               \
			return &root;                           \
		}                                           \
		static MyAccessor* GetAccessor()            \
		{                                           \
			static MyAccessor a(GetRoot());         \
			return &a;                              \
		}                                           \
		MyAccessor* operator -> ()                  \
		{                                           \
			return GetAccessor();                   \
		}                                           \
	};                                              \
	extern StructName       VarName;


 #define CONFIG_VAR_FLOAT( var, def )  ConfVarNumber *const var;
 #define CONFIG_VAR_INT(   var, def )  ConfVarNumber *const var;
 #define CONFIG_VAR_BOOL(  var, def )  ConfVarBool   *const var;
 #define CONFIG_VAR_STR(   var, def )  ConfVarString *const var;
 #define CONFIG_VAR_ARRAY( var, def )  ConfVarArray  *const var;
 #define CONFIG_VAR_TABLE( var, def )  ConfVarTable  *const var;

#elif CONFIG_CACHE_PASS == 2

#undef CONFIG_BEGIN
#undef CONFIG_VAR_FLOAT
#undef CONFIG_VAR_INT
#undef CONFIG_VAR_BOOL
#undef CONFIG_VAR_STR
#undef CONFIG_VAR_ARRAY
#undef CONFIG_VAR_TABLE
#undef CONFIG_END

#define CONFIG_BEGIN(StructName) StructName::MyAccessor::MyAccessor(ConfVarTable *root) \
  : StructName::MyAccessorBase()

#define CONFIG_VAR_FLOAT( var, def )  , var( root->GetNum(#var, (float) (def)) )
#define CONFIG_VAR_INT(   var, def )  , var( root->GetNum(#var, (int) (def)) )
#define CONFIG_VAR_BOOL(  var, def )  , var( root->GetBool(#var, (def)) )
#define CONFIG_VAR_STR(   var, def )  , var( root->GetStr(#var, (def)) )
#define CONFIG_VAR_ARRAY( var, def )  , var( root->GetArray(#var, def) )
#define CONFIG_VAR_TABLE( var, def )  , var( root->GetTable(#var, def) )

#define CONFIG_END(StructName, VarName)  {}               \
    StructName       VarName;

#else
 #error define CONFIG_CACHE_PASS as 1 or 2 before including this file
#endif

#undef CONFIG_CACHE_PASS


// end of file
