//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by ZipReader.hpp, separated for less recompilation
//======================================================================================================================

#ifndef ZIP_READER_TYPES_INCLUDED
#define ZIP_READER_TYPES_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"           // ValueOrError
#include "FileInfoCacheTypes.hpp"  // UncertainFileInfo
#include "MapInfo.hpp"             // MapInfo

#include <QString>


class QJsonObject;
class JsonObjectCtx;


namespace doom {


using UncertainFileContent = ValueOrError<QByteArray, ReadStatus, ReadStatus::Success>;

struct ZipInfo
{
	MapInfo mapInfo;   ///< content extracted from a MAPINFO file

	void serialize( QJsonObject & jsZipInfo ) const;
	void deserialize( const JsonObjectCtx & jsZipInfo );
};

using UncertainZipInfo = UncertainFileInfo< ZipInfo >;


} // namespace doom


#endif // ZIP_READER_TYPES_INCLUDED
