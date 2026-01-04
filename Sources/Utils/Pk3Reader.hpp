//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: pk3 file parsing and information extraction
//======================================================================================================================

#ifndef PK3_READER_INCLUDED
#define PK3_READER_INCLUDED


#include "Essential.hpp"

#include "MapInfo.hpp"  // MapInfo
#include "FileInfoCache.hpp"

#include <QString>


class QJsonObject;
class JsonObjectCtx;



namespace doom {


struct Pk3Info
{
	MapInfo mapInfo;   ///< content extracted from a MAPINFO file

	void serialize( QJsonObject & jsPk3Info ) const;
	void deserialize( const JsonObjectCtx & jsPk3Info );
};

using UncertainPk3Info = UncertainFileInfo< Pk3Info >;

/// Reads selected information from a pk3 file.
/** BEWARE that these file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainPk3Info readPk3Info( const QString & filePath );


} // namespace doom


// cache global for the whole process, because why not
extern FileInfoCache< doom::Pk3Info > g_cachedPk3Info;


#endif // PK3_READER_INCLUDED
