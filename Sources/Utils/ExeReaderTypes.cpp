//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by ExeReader.hpp, separated for less recompilation
//======================================================================================================================

#include "ExeReaderTypes.hpp"

#include "JsonUtils.hpp"


namespace os {


void ExeVersionInfo::serialize( QJsonObject & jsExeInfo ) const
{
	jsExeInfo["app_name"] = appName;
	jsExeInfo["description"] = description;
	jsExeInfo["version"] = version.toString();
}

void ExeVersionInfo::deserialize( const JsonObjectCtx & jsExeInfo )
{
	appName = jsExeInfo.getString("app_name");
	description = jsExeInfo.getString("description");
	version = Version( jsExeInfo.getString("version") );
}


} // namespace os
