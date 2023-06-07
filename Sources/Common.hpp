//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used includes, types and constants
//======================================================================================================================

#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED


#include <memory>
#include <utility>
#include <optional>
#include <cstdint>
#include <climits>

using uint = unsigned int;
using ushort = unsigned short;
using byte = uint8_t;

using std::move;
using std::as_const;

//using namespace std;  // we're working with Qt, so not a good idea

#ifdef _WIN32
	#define IS_WINDOWS true
#else
	#define IS_WINDOWS false
#endif


#endif // COMMON_INCLUDED
