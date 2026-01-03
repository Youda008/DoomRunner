//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information extracted from MAPINFO file either from a WAD or a PK3 file
//======================================================================================================================

#include "MapInfo.hpp"

#include "JsonUtils.hpp"

#include <QIODevice>
#include <QTextStream>
#include <QRegularExpression>


namespace doom {


QJsonObject MapInfo::serialize() const
{
	QJsonObject jsMapInfo;

	jsMapInfo["map_names"] = serializeStringList( mapNames );

	return jsMapInfo;
}

void MapInfo::deserialize( const JsonObjectCtx & jsMapInfo )
{
	if (JsonArrayCtx jsMapNames = jsMapInfo.getArray( "map_names" ))
		mapNames = deserializeStringList( jsMapNames );
}


MapInfo parseMapInfo( const QByteArray & fileContent )
{
	MapInfo mapInfo;

	QTextStream fileText( fileContent, QIODevice::ReadOnly );

	static const QRegularExpression mapDefRegex("^\\s*map\\s+(\\w+)(?:\\s+\"([^\"]*)\")?");

	QString line;
	while (fileText.readLineInto( &line ))
	{
		auto match = mapDefRegex.match( line );
		if (match.hasMatch())
		{
			mapInfo.mapNames.append( match.captured(1) );
		}
	}

	return mapInfo;
}


} // namespace doom
