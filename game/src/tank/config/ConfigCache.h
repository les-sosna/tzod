// ConfigCache.h
// this file is designed to be included twice

#include "ConfigBase.h"

#if CONFIG_CACHE_PASS == 1

 #define CONFIG_BEGIN(StructName)                    \
     struct StructName                               \
     {                                               \
		 static ConfVarTable* GetRoot()              \
		 {                                           \
			 static ConfVarTable root;               \
			 return &root;                           \
		 }                                           \
         struct MyAccessor                           \
         {                                           \
			 MyAccessor(ConfVarTable *root);         \


#define CONFIG_END(StructName, VarName)              \
         };                                          \
         MyAccessor* operator -> ()                  \
         {                                           \
             static MyAccessor a(GetRoot());         \
             return &a;                              \
         }                                           \
     };                                              \
     extern StructName       VarName;


 #define CONFIG_VAR_FLOAT( var, def )  ConfVarNumber *var;
 #define CONFIG_VAR_INT(   var, def )  ConfVarNumber *var;
 #define CONFIG_VAR_BOOL(  var, def )  ConfVarBool   *var;
 #define CONFIG_VAR_STR(   var, def )  ConfVarString *var;
 #define CONFIG_VAR_ARRAY( var )       ConfVarArray  *var;
 #define CONFIG_VAR_TABLE( var )       ConfVarTable  *var;

#elif CONFIG_CACHE_PASS == 2

 #undef CONFIG_BEGIN
 #undef CONFIG_VAR_FLOAT
 #undef CONFIG_VAR_INT
 #undef CONFIG_VAR_BOOL
 #undef CONFIG_VAR_STR
 #undef CONFIG_VAR_ARRAY
 #undef CONFIG_VAR_TABLE
 #undef CONFIG_END

 #define CONFIG_BEGIN(StructName) StructName::MyAccessor::MyAccessor(ConfVarTable *root) {
 #define CONFIG_END(StructName, VarName)  }               \
     StructName       VarName;           \

 #define CONFIG_VAR_FLOAT( var, def )  this->var = root->GetNum(   #var, (float) (def) );
 #define CONFIG_VAR_INT(   var, def )  this->var = root->GetNum(   #var, (int) (def) );
 #define CONFIG_VAR_BOOL(  var, def )  this->var = root->GetBool(  #var, (def) );
 #define CONFIG_VAR_STR(   var, def )  this->var = root->GetStr(   #var, (def) );
 #define CONFIG_VAR_ARRAY( var )       this->var = root->GetArray( #var );
 #define CONFIG_VAR_TABLE( var )       this->var = root->GetTable( #var );

#else
 #error define CONFIG_CACHE_PASS as 1 or 2 before including this file
#endif

#undef CONFIG_CACHE_PASS


// end of file
