//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: pk3 file parsing and information extraction
//======================================================================================================================

#include "Pk3Reader.hpp"

#include "ZipReader.hpp"


namespace doom {


//======================================================================================================================
// JSON serialization

void Pk3Info::serialize( QJsonObject & jsPk3Info ) const
{
	jsPk3Info["map_info"] = mapInfo.serialize();
}

void Pk3Info::deserialize( const JsonObjectCtx & jsPk3Info )
{
	if (JsonObjectCtx jsMapInfo = jsPk3Info.getObject( "map_info" ))
		mapInfo.deserialize( jsMapInfo );
}


//======================================================================================================================
// public API

UncertainPk3Info readPk3Info( const QString & filePath )
{
	UncertainPk3Info pk3Info;

	auto mapInfoContent = readOneOfFilesInsideZip( filePath, { "MAPINFO", "MAPINFO.txt" } );
	if (!mapInfoContent)
	{
		pk3Info.status = mapInfoContent.error();
		return pk3Info;
	}

	pk3Info.mapInfo = doom::parseMapInfo( *mapInfoContent );

	pk3Info.status = !pk3Info.mapInfo.mapNames.isEmpty() ? ReadStatus::Success : ReadStatus::InfoNotPresent;
	return pk3Info;
}


} // namespace doom


FileInfoCache< doom::Pk3Info > g_cachedPk3Info( u"cachedPk3Info", doom::readPk3Info );
