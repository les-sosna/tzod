// stdafx.h
///////////////////////////////////////////////////////////////////////////////

#define _WIN32_WINDOWS  0x0410 // windows 98
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_RAND_S  
#define VC_EXTRALEAN

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


#include "ui/ConsoleBuffer.h"
UI::ConsoleBuffer& GetConsole();


#include "core/types.h"
#include "core/MyMath.h"
#include "core/MemoryManager.h"
#include "core/singleton.h"
#include "core/PtrList.h"
#include "core/SafePtr.h"
#include "core/Grid.h"
#include "core/Delegate.h"

#include "constants.h"
#include "globals.h"

#include "md5.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
// direct x
# define DIRECTINPUT_VERSION 0x0800
# include <dinput.h>
# include "dsutil.h"
// memory leaks detection
# define _CRTDBG_MAP_ALLOC
# include <stdlib.h>
# include <crtdbg.h>
# include <winsock2.h>
# include <windows.h>
# include <commctrl.h>
# include <io.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <time.h>

// c++
#include <cassert>
#include <functional>
#include <cmath>
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
#include <cfloat>
#include <ios>
#include <chrono>
#include <memory>
#include <thread>

// ogg/vorbis
#ifndef NOSOUND
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

// lua
extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

// zlib
#include <zlib.h>



///////////////////////////////////////////////////////////////////////////////
// end of file
