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

struct WadInfo_
{
	WadType type = WadType::Neither;
	QStringVec mapNames;

	void serialize( QJsonObject & jsWadInfo ) const;
	void deserialize( const JsonObjectCtx & jsWadInfo );
};

using WadInfo = UncertainFileInfo< WadInfo_ >;

/// Reads selected information from a WAD file.
/** BEWARE that on file I/O operations may sometimes be expensive, caching the info is adviced. */
WadInfo readWadInfo( const QString & filePath );


extern FileInfoCache< WadInfo_ > g_cachedWadInfo;


} // namespace doom


#endif // WAD_READER_INCLUDED
