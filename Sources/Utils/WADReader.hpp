//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#ifndef WAD_READER_INCLUDED
#define WAD_READER_INCLUDED


#include "Essential.hpp"

#include "DoomFiles.hpp"  // GameIdentification
#include "MapInfo.hpp"  // MapInfo
#include "FileInfoCache.hpp"

#include <QString>

class QJsonObject;
class JsonObjectCtx;


namespace doom {


enum class WadType
{
	Neither,
	IWAD,
	PWAD,
};

struct WadInfo
{
	WadType type = WadType::Neither;
	GameIdentification game;   ///< which game it probably is, only present if the type == IWAD
	MapInfo mapInfo;           ///< content extracted from a MAPINFO file

	void serialize( QJsonObject & jsWadInfo ) const;
	void deserialize( const JsonObjectCtx & jsWadInfo );
};

using UncertainWadInfo = UncertainFileInfo< WadInfo >;

/// Reads selected information from a WAD file.
/** BEWARE that these file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainWadInfo readWadInfo( const QString & filePath );


} // namespace doom


// cache global for the whole process, because why not
extern FileInfoCache< doom::WadInfo > g_cachedWadInfo;


#endif // WAD_READER_INCLUDED
