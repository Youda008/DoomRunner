//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: zip file parsing and information extraction
//======================================================================================================================

#ifndef ZIP_READER_INCLUDED
#define ZIP_READER_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"           // ValueOrError
#include "MapInfo.hpp"             // MapInfo
#include "FileInfoCache.hpp"

#include <QString>


class QJsonObject;
class JsonObjectCtx;



namespace doom {


using UncertainFileContent = ValueOrError<QByteArray, ReadStatus, ReadStatus::Success>;

/// Extracts the content of the first of innerFileNames that is found within the zip file.
/** BEWARE that this operation may be very time consuming, depending on the size of the file and level of compression.
  * Doing this asynchronously is adviced.
  * The returned status will be NotFound when the zip file is not found,
  * but InfoNotPresent when none of the innerFileNames is found. */
UncertainFileContent readOneOfFilesInsideZip( const QString & zipFilePath, const QStringList & innerFileNames );


struct ZipInfo
{
	MapInfo mapInfo;   ///< content extracted from a MAPINFO file

	void serialize( QJsonObject & jsZipInfo ) const;
	void deserialize( const JsonObjectCtx & jsZipInfo );
};

using UncertainZipInfo = UncertainFileInfo< ZipInfo >;

/// Reads selected information from a zip file.
/** BEWARE that these file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainZipInfo readZipInfo( const QString & filePath );


} // namespace doom


// cache global for the whole process, because why not
extern FileInfoCache< doom::ZipInfo > g_cachedZipInfo;


#endif // ZIP_READER_INCLUDED
