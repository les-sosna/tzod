// Macros.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_KILL(p)         { if(p&&!p->IsKilled()) (p)->Kill(); (p)=NULL; }

///////////////////////////////////////////////////////////////////////////////

#define FOREACH(ls, type, var)                                          \
	if( type *var = (type *) (0xffffffff) )                             \
	for( ObjectList::iterator var##iterator = (ls).begin();             \
	         (var = static_cast<type *>(*var##iterator)),               \
	         (var##iterator != (ls).end());                             \
	    var##iterator++)

#define FOREACH_SAFE(list, type, var)                                        \
    if( type *var = (type *) -1 )                                            \
        for( ObjectList::safe_iterator var##iterator = (list).safe_begin();  \
             (var = static_cast<type *>(*var##iterator)),                    \
             (var##iterator != (list).end()); ++var##iterator )

#define FOREACH_R(ls, type, var)                                        \
	if( type *var = (type *) (0xffffffff) )                             \
	for( ObjectList::reverse_iterator var##iterator = (ls).rbegin();   \
	      (var = static_cast<type *>(*var##iterator)),                  \
	      (var##iterator != (ls).rend());                               \
	    var##iterator++ )


///////////////////////////////////////////////////////////////////////////////

#define WIDTH(rect) (rect.right - rect.left)
#define HEIGHT(rect) (rect.bottom - rect.top)

///////////////////////////////////////////////////////////////////////////////
// end of file
