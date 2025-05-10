//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by WADReader.hpp, separated for less recompilation
//======================================================================================================================

#ifndef WAD_READER_TYPES_INCLUDED
#define WAD_READER_TYPES_INCLUDED


#include "Essential.hpp"

#include "FileInfoCacheTypes.hpp"  // UncertainFileInfo
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


} // namespace doom


#endif // WAD_READER_TYPES_INCLUDED
