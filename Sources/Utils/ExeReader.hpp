//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: executable file parsing and information extraction
//======================================================================================================================

#ifndef EXE_READER_INCLUDED
#define EXE_READER_INCLUDED


#include "Essential.hpp"

#include "ExeReaderTypes.hpp"

#include "FileInfoCache.hpp"


namespace os {


/// Reads executable version info from the file's built-in resource.
/** Even if status == Success, not all the fields have to be filled. If the version info resource was found,
  * but some of the expected entries is not present, the corresponding ExeVersionInfo field will remain empty/invalid.
  * BEWARE that on some systems opening the executable file can take incredibly long, so caching is strongly adviced. */
UncertainExeVersionInfo readExeVersionInfo( const QString & filePath );


} // namespace os


// cache global for the whole process, because why not
extern FileInfoCache< os::ExeVersionInfo > g_cachedExeInfo;


#endif // EXE_READER_INCLUDED
