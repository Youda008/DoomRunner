//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by ZipReader.hpp, separated for less recompilation
//======================================================================================================================

#include "ZipReaderTypes.hpp"

#include "JsonUtils.hpp"


namespace doom {


void ZipInfo::serialize( QJsonObject & jsZipInfo ) const
{
	jsZipInfo["map_info"] = mapInfo.serialize();
}

void ZipInfo::deserialize( const JsonObjectCtx & jsZipInfo )
{
	if (JsonObjectCtx jsMapInfo = jsZipInfo.getObject( "map_info" ))
		mapInfo.deserialize( jsMapInfo );
}


} // namespace doom
