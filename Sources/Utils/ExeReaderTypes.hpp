//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by ExeReader.hpp, separated for less recompilation
//======================================================================================================================

#ifndef EXE_READER_TYPES_INCLUDED
#define EXE_READER_TYPES_INCLUDED


#include "Essential.hpp"

#include "FileInfoCacheTypes.hpp"  // UncertainFileInfo
#include "Version.hpp"

#include <QString>

class QJsonObject;
class JsonObjectCtx;


namespace os {


/// Executable version information
struct ExeVersionInfo
{
	QString appName;
	QString description;
	Version version;

	void serialize( QJsonObject & jsExeInfo ) const;
	void deserialize( const JsonObjectCtx & jsExeInfo );
};

using UncertainExeVersionInfo = UncertainFileInfo< ExeVersionInfo >;


} // namespace os


#endif // EXE_READER_TYPES_INCLUDED
