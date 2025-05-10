//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by WADReader.hpp, separated for less recompilation
//======================================================================================================================

#include "WADReaderTypes.hpp"

#include "JsonUtils.hpp"


namespace doom {


void WadInfo::serialize( QJsonObject & jsWadInfo ) const
{
	jsWadInfo["type"] = int( type );
	jsWadInfo["map_names"] = serializeStringList( mapNames );
	// TODO: game identification
}

void WadInfo::deserialize( const JsonObjectCtx & jsWadInfo )
{
	type = jsWadInfo.getEnum< doom::WadType >( "type", doom::WadType::Neither );
	if (JsonArrayCtx jsMapNames = jsWadInfo.getArray( "map_names" ))
		mapNames = deserializeStringList( jsMapNames );
	// TODO: game identification
}


} // namespace doom
