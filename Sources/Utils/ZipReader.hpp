//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: zip file parsing and information extraction
//======================================================================================================================

#ifndef ZIP_READER_INCLUDED
#define ZIP_READER_INCLUDED


#include "Essential.hpp"

#include "ZipReaderTypes.hpp"

#include "FileInfoCache.hpp"


namespace doom {


/// Extracts the content of the first of innerFileNames that is found within the zip file.
/** BEWARE that this operation may be very time consuming, depending on the size of the file and level of compression.
  * Doing this asynchronously is adviced.
  * The returned status will be NotFound when the zip file is not found,
  * but InfoNotPresent when none of the innerFileNames is found. */
UncertainFileContent readOneOfFilesInsideZip( const QString & zipFilePath, const QStringList & innerFileNames );

/// Reads selected information from a zip file.
/** BEWARE that these file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainZipInfo readZipInfo( const QString & filePath );


// cache global for the whole process, because why not
extern FileInfoCache< ZipInfo > g_cachedZipInfo;


} // namespace doom


#endif // ZIP_READER_INCLUDED
