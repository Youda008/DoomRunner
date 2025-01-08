//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#ifndef WAD_READER_INCLUDED
#define WAD_READER_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"
#include "FileInfoCache.hpp"
#include "DoomFiles.hpp"  // GameIdentification

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
	QStringList mapNames;       ///< list of map names usable for the +map command

	void serialize( QJsonObject & jsWadInfo ) const;
	void deserialize( const JsonObjectCtx & jsWadInfo );
};

using UncertainWadInfo = UncertainFileInfo< WadInfo >;

/// Reads selected information from a WAD file.
/** BEWARE that on file I/O operations may sometimes be expensive, caching the info is adviced. */
UncertainWadInfo readWadInfo( const QString & filePath );


// cache global for the whole process, because why not
extern FileInfoCache< WadInfo > g_cachedWadInfo;


} // namespace doom


#endif // WAD_READER_INCLUDED
