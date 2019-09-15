#include "inc/config/ConfigCache.h"

ConfVarNumber& InitIntType(ConfVarNumber &var, const char *type, const char *help)
{
	//	if( type )
	//	{
	//		ConfVarNumber::NumberMeta meta;
	//		meta.type = type;
	//		var->SetMeta(&meta);
	//	}
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarNumber& InitIntRange(ConfVarNumber &var, int iMin, int iMax, const char *help)
{
	//	ConfVarNumber::NumberMeta meta;
	//	meta.type = "integer";
	//	meta.fMin = (float) iMin;
	//	meta.fMax = (float) iMax;
	//	var->SetMeta(&meta);
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarNumber& InitFloat(ConfVarNumber &var, const char *help)
{
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarBool& InitBool(ConfVarBool &var, const char *help)
{
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarArray& InitArray(ConfVarArray &var, const char *help)
{
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarTable& InitTable(ConfVarTable &var, const char *help)
{
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarNumber& InitFloatRange(ConfVarNumber &var, float fMin, float fMax, const char *help)
{
	//	ConfVarNumber::NumberMeta meta;
	//	meta.type = "float";
	//	meta.fMin = fMin;
	//	meta.fMax = fMax;
	//	var->SetMeta(&meta);
	//	if( help ) var->SetHelpString(help);
	return var;
}

ConfVarString& InitStrType(ConfVarString &var, const char *type, const char *help)
{
	//	if( type )
	//	{
	//		ConfVarString::StringMeta meta;
	//		meta.type = type;
	//		var->SetMeta(&meta);
	//	}
	//	if( help ) var->SetHelpString(help);
	return var;
}
