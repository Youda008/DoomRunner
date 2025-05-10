//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Windows-specific utils
//              In their own module, because they require including windows.h, and we want to limit the reach of that.
//======================================================================================================================

#ifndef WIN_UTILS_INCLUDED
#define WIN_UTILS_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"  // ValueOrError

#include <windows.h>

constexpr HKEY INVALID_HKEY = nullptr;


namespace win {


//----------------------------------------------------------------------------------------------------------------------
// registry

using OptRegistryKey = ValueOrError< HKEY, LONG, ERROR_SUCCESS >;
OptRegistryKey openRegistryKey( HKEY parentKeyHandle, const char * subKeyPath );
void closeRegistryKey( HKEY keyHandle );

template< typename Value >
using OptRegistryValue = ValueOrError< Value, LONG, ERROR_SUCCESS >;
OptRegistryValue< DWORD > readRegistryDWORD( HKEY parentKeyHandle, const char * subkeyPath, const char * valueName );

LONG waitForRegistryKeyChange( HKEY keyHandle );


} // namespace win


#endif // WIN_UTILS_INCLUDED
