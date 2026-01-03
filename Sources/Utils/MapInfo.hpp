//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information extracted from MAPINFO file either from a WAD or a PK3 file
//======================================================================================================================

#ifndef MAPINFO_INCLUDED
#define MAPINFO_INCLUDED


#include "Essential.hpp"

#include <QStringList>

class QJsonObject;
class JsonObjectCtx;
class QByteArray;


namespace doom {


struct MapInfo
{
	QStringList mapNames;   ///< list of map names usable for the +map command

	QJsonObject serialize() const;
	void deserialize( const JsonObjectCtx & jsWadInfo );
};


MapInfo parseMapInfo( const QByteArray & fileContent );


} // namespace doom


#endif // MAPINFO_INCLUDED
