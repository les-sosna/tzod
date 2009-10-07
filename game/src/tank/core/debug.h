// debug.h

#pragma once


//#ifdef _DEBUG
#define TRACE(fmt, ...) GetConsole().Printf(0, fmt, __VA_ARGS__);
//#else
//#define TRACE
//#endif



// generates an error if an invalid class name passed
#ifdef _DEBUG
#define CLS(name) (sizeof(name), name::this_type)
#else
#define CLS(name) (name::this_type)
#endif


#define ASSERT_TYPE(pointer, type) assert(NULL != dynamic_cast<type*>(pointer))

///////////////////////////////////////////////////////////////////////////////
// end of file
