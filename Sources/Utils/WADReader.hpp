//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#ifndef WAD_READER_INCLUDED
#define WAD_READER_INCLUDED


#include "Essential.hpp"

#include "WADReaderTypes.hpp"

#include "FileInfoCache.hpp"


namespace doom {


/// Reads selected information from a WAD file.
/** BEWARE that these file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainWadInfo readWadInfo( const QString & filePath );


// cache global for the whole process, because why not
extern FileInfoCache< WadInfo > g_cachedWadInfo;


} // namespace doom


#endif // WAD_READER_INCLUDED
