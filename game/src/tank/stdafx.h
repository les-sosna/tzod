// stdafx.h
///////////////////////////////////////////////////////////////////////////////

#define _WIN32_WINDOWS  0x0410 // windows 98

#define _CRT_SECURE_NO_WARNINGS

#define VC_EXTRALEAN
//#define WIN32_LEAN_AND_MEAN


#pragma warning (disable: 4355) // 'this' : used in base member initializer list


///////////////////////////////////////////////////////////////////////////////
//  If defined, the following flags inhibit definition of the indicated items.

#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOICONS           // IDI_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
//#define NOCLIPBOARD       // Clipboard routines
#define NOKERNEL          // All KERNEL defines and routines
#define NONLS             // All NLS defines and routines
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
//#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

//#define NOSOUND           // Sound driver routines

///////////////////////////////////////////////////////////////////////////////

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <crtdbg.h>
#include <fcntl.h>
#include <io.h>

#include <math.h>
#include <time.h>

// c++ libraries

#ifndef _DEBUG
#define _SECURE_SCL 0  // disable checked iterators
#endif

#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>

#include <ios>

///////////////
// direct x

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"

#include "dsutil.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#pragma comment(lib, "oggvorbis.lib")



#ifdef _DEBUG
#define LUA_USE_APICHECK
#endif

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#pragma comment(lib, "lua.lib")

///////////////////
// other stuff

#include "core/types.h"

#include "core/MyMath.h"

#include "core/MemoryManager.h"
#include "core/singleton.h"
#include "core/PtrList.h"
#include "core/SafePtr.h"

#include "core/Grid.h"

#include "core/Delegate.h"


#include "globals.h"


///////////////////////////////////////////////////////////////////////////////
// end of file
