//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Windows-specific utils
//              In their own module, because they require including windows.h, and we want to limit the reach of that.
//======================================================================================================================

#if IS_WINDOWS


#include "WindowsUtils.hpp"


namespace win {


//======================================================================================================================
// registry

OptRegistryKey openRegistryKey( HKEY parentKeyHandle, const char * subKeyPath )
{
	HKEY keyHandle = INVALID_HKEY;
	LONG error = RegOpenKeyExA(
		parentKeyHandle, subKeyPath,
		0,                             // in: options
		KEY_QUERY_VALUE | KEY_NOTIFY,  // in: requested permissions
		&keyHandle                     // out: handle to the opened key
	);
	return { keyHandle, error };
}

void closeRegistryKey( HKEY keyHandle )
{
	RegCloseKey( keyHandle );
}

OptRegistryValue< DWORD > readRegistryDWORD( HKEY parentKeyHandle, const char * subKeyPath, const char * valueName )
{
	DWORD value = 0;
	DWORD varLength = DWORD( sizeof(value) );
	LONG error = RegGetValueA(
		parentKeyHandle, subKeyPath, valueName,
		RRF_RT_REG_DWORD,  // in: value type filter - only accept DWORD
		nullptr,           // out: final value type
		&value,            // out: the requested value
		&varLength         // in/out: size of buffer / number of bytes read
	);
	return { value, error };
}

LONG waitForRegistryKeyChange( HKEY keyHandle )
{
	LONG error = RegNotifyChangeKeyValue(
		keyHandle,                   // handle to the opened key
		FALSE,                       // monitor subtree
		REG_NOTIFY_CHANGE_LAST_SET,  // notification filter
		nullptr,                     // handle to event object
		FALSE                        // asynchronously
	);
	return error;
}


} // namespace win


#endif // IS_WINDOWS
