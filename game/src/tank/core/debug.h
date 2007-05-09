// debug.h

#pragma once


#ifndef _DEBUG
#define _CrtDbgReport
#endif

#if defined _DEBUG
#define REPORT(str) {_CrtDbgReport(_CRT_WARN, NULL, NULL, NULL, (str)); LOGOUT_1(str);}
#else
#define REPORT
#endif


// generates an error if an invalid class name passed
#ifdef _DEBUG
#define _CLS(name) (sizeof(name), name::this_type)
#else
#define _CLS(name) (name::this_type)
#endif


#define ASSERT_TYPE(pointer, type) _ASSERT(NULL != dynamic_cast<type*>(pointer))

//-----------------------------------

#ifdef LOGFILE
 extern FILE *__g_plogfile;

 BOOL InitLogFile(char *fileName);

 #define LOGOUT_1(p1) {fprintf(__g_plogfile, (p1)); fflush(__g_plogfile);}
 #define LOGOUT_2(p1, p2) {fprintf(__g_plogfile, (p1), (p2)); fflush(__g_plogfile);}
 #define LOGOUT_3(p1, p2, p3) {fprintf(__g_plogfile, (p1), (p2), (p3)); fflush(__g_plogfile);}
#else
 #define InitLogFile
 #define LOGOUT_1
 #define LOGOUT_2
 #define LOGOUT_3
#endif


///////////////////////////////////////////////////////////////////////////////
// end of file
