//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for generic enum processing
//======================================================================================================================

#ifndef ENUM_TRAITS_INCLUDED
#define ENUM_TRAITS_INCLUDED


template< typename Enum >
const char * enumName() { return "unknown"; }

template< typename Enum >
decltype( sizeof(int) )/* instead of including size_t */ enumSize() { return 0; }


#endif // ENUM_TRAITS_INCLUDED
